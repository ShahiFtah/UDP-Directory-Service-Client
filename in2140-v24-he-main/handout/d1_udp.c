/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "d1_udp.h"

uint16_t calculate_checksum(const void *buff, size_t len);

D1Peer* d1_create_client( )
{
    
    D1Peer* client = (D1Peer*)malloc(sizeof(D1Peer));
    if (client == NULL) {
        perror("Memory failed");
        return NULL;
    }

    
    client->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->socket < 0) {
        perror("socket");
        free(client); // Renser minne
        return NULL;
    }
    
    // Setter adresse til 0
    memset(&(client->addr), 0, sizeof(struct sockaddr_in));
    
    // setter next_seqno til 0
    client->next_seqno = 0;
    return client;
}

D1Peer* d1_delete(D1Peer* peer) { 
    if (peer != NULL) {
        //sjekker om peer er tom
        if (peer->socket >= 0) {
            close(peer->socket);
        }
        // løser minne ved å bruk free
        free(peer);
    }
    // returnerer NULL 
    return NULL;
}

int d1_get_peer_info(struct D1Peer* peer, const char* peername, uint16_t server_port) {
    if (peer == NULL) {
        return 0; 
    }

    memset(&(peer->addr), 0, sizeof(peer->addr));
    peer->addr.sin_family = AF_INET;
    peer->addr.sin_port = htons(server_port);

    // Konvertere peername fra tekst til binaer 
    if (inet_pton(AF_INET, peername, &peer->addr.sin_addr) != 1) {
        //hvis ikke kan vi teste den slik:
        struct hostent *host = gethostbyname(peername);
        if (host == NULL) {
            return 0; //fant ikke hostnavn
        }
        // kopierer adresse til adresse-strukturen
        memcpy(&(peer->addr.sin_addr), host->h_addr, host->h_length);
    }

    return 1; //Dette betyr godkjent
}

int d1_recv_data(struct D1Peer* peer, char* buffer, size_t sz) {
    
    if (!peer) {
        return -1; // tom peer
    }

    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // Mottar pakke fra serveren ved å bruke recvfrom
    ssize_t recv_len = recvfrom(peer->socket, buffer, sz, 0, (struct sockaddr*)&sender_addr, &sender_len);

    if (recv_len <= 0) {
        if (recv_len < 0) {
            perror("recvfrom failed");
            return -1;
        } else {
            return 0; // tom data-pakke
        }
    }

    // sjekker størrelsen
    if ((size_t)recv_len < sizeof(D1Header)) {
        fprintf(stderr, "Received packet er for liten\n");
        d1_send_ack(peer, (peer->next_seqno + 1) % 2); // Send ACK med motsatt sekvens
        return -1; // pakken er for liten
    }
    
    D1Header* header = (D1Header*)buffer; // Konverterer buffer til en D1Header-struktur

    // Validerer checksum
    uint16_t checksum = header->checksum;
    header->checksum = 0; // Nullstill checksum før beregning
    uint16_t calculated_checksum = 0;

    // Beregner checksum ved å XORere alle ord i bufferen
    for (size_t i = 0; i < recv_len / 2; ++i) {
        calculated_checksum ^= ((uint16_t*)buffer)[i];
    }

    // Sjekker om den beregnede checksummen stemmer overens med den mottatte checksummen
    if (checksum != calculated_checksum) {
        fprintf(stderr, "Checksum  ulik\n");
        d1_send_ack(peer, (peer->next_seqno + 1) % 2); // Sender ACK med motsatt sekvensnummer
        return -1; // Feil: checksum mismatch
    }

    // sjekker hvis received packet er en ACK packet
    if (ntohs(header->flags) & FLAG_ACK) {
        printf("Mottat ACK header flags: 0x%X\n", ntohs(header->flags));
        printf("Mottat ACK sequence number: %d\n", ntohs(header->flags) & ACKNO);
    }

    
    size_t payload_size = recv_len - sizeof(D1Header);
    memmove(buffer, buffer + sizeof(D1Header), payload_size); 
    buffer[payload_size] = '\0'; // Null-terminere buffer, for å ikke skape problemer i videre utregninger

    
    d1_send_ack(peer, peer->next_seqno);
    peer->next_seqno ^= 1;
    return payload_size;
}

int d1_wait_ack(D1Peer* peer, char* buffer, size_t sz) { //Her setter vi opp en tidsbegrensning hvor vi venter i "tid" til pakken kommer, dette er i evig løkke
    printf("Waiting for ACK\n");
    struct timeval timeout;
    timeout.tv_sec = 1;  
    timeout.tv_usec = 0;

    if (setsockopt(peer->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        return -1;
    }

    while (1) {
        D1Header ack_header;
        ssize_t bytes_received = recv(peer->socket, &ack_header, sizeof(ack_header), 0);
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  
            } else {
                perror("Error paa mottak av ACK");
                return -1;
            }
        }

        printf("Mottat ACK header flags: 0x%X\n", ntohs(ack_header.flags));

        int received_seqno = ntohs(ack_header.flags) & ACKNO;
        printf("Mottat ACK seqno: %d, Forventet ACK seqno: %d\n", received_seqno, peer->next_seqno);

        if (received_seqno == peer->next_seqno && (ntohs(ack_header.flags) & FLAG_ACK)) {
            printf("Mottat ACK med riktig sequence number\n");
            peer->next_seqno ^= 1;  
            return 1;
        } else {
            printf("Uforventet sequence number i ACK. Forventet %d, Mottat %d\n", peer->next_seqno, received_seqno);
            continue;
        }
    }
}

int d1_send_data(struct D1Peer* peer, char* buffer, size_t sz) {
    if (!peer) {
        fprintf(stderr, "Error: peer pointer er NULL\n");
        return -1;
    }

    if (sz > (1024 - sizeof(D1Header))) {
        fprintf(stderr, "Error: data size gaar over maximum-grense\n");
        return -1;
    }

    char packet[1024] = {0};
    size_t total_size = sizeof(D1Header) + sz;
    if (total_size > 1024) {
        fprintf(stderr, "Error: total packet size gaar over buffer size\n");
        return -1;
    }

    D1Header header;
    header.flags = htons(FLAG_DATA | (peer->next_seqno << 7));
    header.size = htonl(total_size);
    header.checksum = 0; // plassholder til vi regner ut lenger ned

    //uint8_t packet[1024]; trenger ikke
    memcpy(packet, &header, sizeof(D1Header));
    memcpy(packet + sizeof(D1Header), buffer, sz);

    // Kaller på funksjonen calculate_checksum som regner ut checksum
    header.checksum = calculate_checksum(packet, total_size);
    memcpy(packet, &header, sizeof(D1Header));

    printf("Packet data (hex): ");
    for (int i = 0; i < total_size; i++) {
        printf("%02x ", (unsigned char)packet[i]);
    }
    printf("\n");

    // Send pakke
    ssize_t bytes_sent = sendto(peer->socket, packet, total_size, 0, (struct sockaddr*)&(peer->addr), sizeof(peer->addr));
    if (bytes_sent < 0) {
        perror("Error sending data");
        return -1;
    }
    // Vent på ACK
    int ack = d1_wait_ack(peer, buffer, sz);
    if (ack < 0) {
        fprintf(stderr, "Error waiting for ACK\n");
        return -1;
    }
    peer->next_seqno ^= 1;
    return bytes_sent;
}

void d1_send_ack(struct D1Peer* peer, int seqno) {
    if (peer == NULL) {
        fprintf(stderr, "Error: Peer pointer is NULL\n");
        return;
    }
    
    // lag en ACK packet
    D1Header ack_header;
    ack_header.flags = htons(FLAG_ACK | seqno); 
    ack_header.size = htonl(sizeof(D1Header));
    ack_header.checksum = 0;

    // regne checksum for ACK packet
    uint16_t checksum = calculate_checksum(&ack_header, sizeof(D1Header)); 
    ack_header.checksum = checksum;

    // Send ACK packet til peer adressen
    ssize_t bytes_sent = sendto(peer->socket, (const char*)&ack_header, sizeof(D1Header), 0, (struct sockaddr*)&(peer->addr), sizeof(peer->addr));
    if (bytes_sent < 0) {
        perror("send til ACK feilet");
        return;
    }
    printf("\nSENDER ACKPAKKE\n");
}


uint16_t calculate_checksum(const void *buff, size_t len)
{
    uint16_t checksum = 0;
    const uint16_t *data = buff;
    // Checksum utregning inkluderer alle headere-områder 
    for (size_t i = 0; i < (len / 2); i++)
    {
        checksum ^= data[i];
    }
    // utfoor arbeidet dersom verdien er odde
    if (len % 2)
    {
        checksum ^= ((uint8_t *)data)[len - 1];
    }
    return checksum;
}
