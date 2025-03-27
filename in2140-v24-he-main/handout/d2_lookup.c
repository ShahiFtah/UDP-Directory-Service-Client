/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "d2_lookup.h"
#include "d1_udp.h"

D2Client* d2_client_create( const char* server_name, uint16_t server_port )
{
    D2Client* client = (D2Client*)malloc(sizeof(D2Client));
    if (client == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    //registrerer koblig ved å kalle på d1 funksjonen
    client->peer = d1_create_client();
    if (client->peer == NULL) {
        fprintf(stderr, "Feilet til å lage D1Peer\n");
        free(client); //rens hele minnet
        return NULL;
    }
    
    // Her bruekr vi D1 function for å finne ut informasjon om peer/server slik at vi vet at den er safe
    if (!d1_get_peer_info(client->peer, server_name, server_port)) {
        fprintf(stderr, "Failed to get peer info\n");
        d1_delete(client->peer); // Rens minnet
        free(client); 
        return NULL;
    }
    
    return client;
}

D2Client* d2_client_delete( D2Client* client )
{
    if (client != NULL) {
        //Oppsettet er tatt fra d1, her gjør de akkurat det samme
        if (client->peer != NULL) {
            d1_delete(client->peer);
            client->peer = NULL;
        }
        free(client);
    }
    return NULL;
}

int d2_send_request(D2Client* client, uint32_t id) {
    if (client == NULL) {
        fprintf(stderr, "Error: Prover å sende request med en NULL client\n");
        return -1;
    }
    // lager en packetRequest struct med angitt data fra parameteren 
    PacketRequest request;
    request.type = htons(TYPE_REQUEST);
    request.id = htonl(id);
    ssize_t sent_bytes = d1_send_data(client->peer, (char*)&request, sizeof(PacketRequest));
    if (sent_bytes < 0) {
        perror("Error sending request data");
        return -1;
    }
    return (int)sent_bytes; //returnerer antall bytes som er sendt fra d1 funksjonen: d1_send_data
}
int d2_recv_response_size(D2Client* client) {
    char packet_buffer[1024];
    if (client == NULL) {
        fprintf(stderr, "Error: prover å sende receive response size med en NULL client\n");
        return -1;
    }
    int received_bytes = d1_recv_data(client->peer, packet_buffer, sizeof(packet_buffer));
    if (received_bytes == 0) {
        printf("laast tilkobling av peer\n");
        return 0;
    }
    if (received_bytes < 0) {
        fprintf(stderr, "Error mottak av data\n");
        return -1;
    }
    PacketResponseSize* response_size_packet = (PacketResponseSize*)packet_buffer;
    uint16_t packet_type = ntohs(response_size_packet->type);
    if (packet_type != TYPE_RESPONSE_SIZE) {
        fprintf(stderr, "Mottat pakke er ikke en PacketResponseSize\n");
        return -1;
    }
    uint16_t size_host_order = ntohs(response_size_packet->size);
    printf("Received response size: %d\n", size_host_order);
    return size_host_order;
}
int d2_recv_response(D2Client* client, char* buffer, size_t sz) {
    if (client == NULL || buffer == NULL) { // hvis klienten eller buffere ner null: ikke godkjent
        fprintf(stderr, "Error: ugyldig argument for mottak av response pakker\n");
        return -1;
    }
    int received_bytes = d1_recv_data(client->peer, buffer, sz);
    if (received_bytes < 0) {
        // Error mottak av data, returnerer mottat bytes
        perror("Error paa mottak av response data");
        return received_bytes;
    }
    // Returnerer antall bytes fra d1 funksjonen
    return received_bytes;
}

LocalTreeStore* d2_alloc_local_tree(int num_nodes) {
    if (num_nodes <= 0) {
        fprintf(stderr, "Error: Antall nodes maa vaere positive.\n");
        return NULL;
    }
    LocalTreeStore* store = (LocalTreeStore*)malloc(sizeof(LocalTreeStore));
    if (!store) {
        perror("Feilet til a allokere LocalTreeStore");
        return NULL;
    }
    store->nodes = (NetNode*)calloc(num_nodes, sizeof(NetNode));
    if (!store->nodes) {
        perror("Feilet til a allokere minne for nodes");
        free(store);
        return NULL;
    }

    store->number_of_nodes = num_nodes;
    return store;
}

// Enkel funksjon som sletter alle calloc/malloc minnene slik at vi ikke får noen leaks
void d2_free_local_tree(LocalTreeStore* nodes) {
    if (nodes) {
        free(nodes->nodes);
        free(nodes);
    }
}

//Denne funksjonen legger til nettverksnoder i en lokal trestruktur ved å kopiere nodedata fra et buffer og sette dem inn i riktig format i trestrukturen. Den håndterer nettverksbyteordenkonvertering og sjekker for feil underveis.
int d2_add_to_local_tree(LocalTreeStore *store, int node_idx, char *buffer, int buflen)
{
    if (store == NULL || buffer == NULL || buflen < 0)
    {
        fprintf(stderr, "feil eller ugyldige parameter(e) \n");
        return -1;
    }
    int verdi = 0; 
    while (verdi < buflen)
    {
        if (node_idx >= store->number_of_nodes)
        {
            fprintf(stderr, "Verdien til Node er større en antall noder i treet\n"); 
            return -1;
        }
        NetNode *node = &store->nodes[node_idx];
        memcpy(&node->id, buffer + verdi, sizeof(node->id));
        verdi += sizeof(uint32_t);
        node->id = ntohl(node->id);
        memcpy(&node->value, buffer + verdi, sizeof(node->value));
        verdi += sizeof(uint32_t);
        node->value = ntohl(node->value);
        memcpy(&node->num_children, buffer + verdi, sizeof(node->num_children));
        verdi += sizeof(uint32_t);
        node->num_children = ntohl(node->num_children);
        for (uint32_t i = 0; i < node->num_children; i++) 
        {
            if (verdi >= buflen)
            {
                fprintf(stderr, "Bufferet er fullt, ikke mer plass\n"); 
                return -1;
            }
            memcpy(&node->child_id[i], buffer + verdi, sizeof(uint32_t));
            node->child_id[i] = ntohl(node->child_id[i]);
            verdi += sizeof(uint32_t);
        }
        node_idx++;
    }
    return node_idx;
}
 

void d2_print_tree(LocalTreeStore* nodes) {
    if (!nodes || !nodes->nodes) {
        printf("Treet er tom/ikke initialisert\n");
        return;
    }
    for (int i = 0; i < nodes->number_of_nodes; i++) {
        NetNode* node = &nodes->nodes[i];
        for (int depth = 0; depth < i; depth++) printf("--");
        printf("id %u value %u children %u\n", node->id, node->value, node->num_children);
    }
}

void print_tree_recursive(LocalTreeStore *store, int node_index, int depth)
{
    if (node_index < 0 || node_index >= store->number_of_nodes)
    {
        return;
    }
    NetNode node = store->nodes[node_index];
    for (int i = 0; i < depth; i++)
    {
        printf("--");
    }
    printf("id %d value %d children %d\n", node.id, node.value, node.num_children);
    for (uint32_t i = 0; i < node.num_children; i++)
    {
        int child_indeks = node.child_id[i];
        print_tree_recursive(store, child_indeks, depth + 1);
    }
}

/*Den første funksjonen, d2_print_tree, skriver ut innholdet i en trestruktur ved å traversere gjennom alle nodene og 
skrive ut deres ID, verdi og antall barn. Hvis treet er tomt eller ikke initialisert, skrives det ut en passende melding.

Den andre funksjonen, print_tree_recursive, utfører en rekursiv dybdeførst gjennomgang av treet ved å skrive ut hver node 
og dens barn. Den håndterer også tilfeller der treet er tomt eller noden ikke har gyldige barn.*/