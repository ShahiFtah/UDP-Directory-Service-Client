IN2140 Introduksjon til operativsystemer og datakommunikasjon
Kandidatnummer: 15032

I denne oppgaven inneholder implementasjonene av to c-filer: d1_udp.c og d2_lookup.c.
Formålet med oppgaven er å lage og legge til funksjonalitet for å opprette UDP-kobling, 
slik at vi kan sende pakker til og fra mellom klient og serveren hvor hver pakke godkjennes
av den motsatte siden ved å sende separate ACK-pakker, kommunisere over nettverket ved hjelp av
UDP-protokollen, og utføre forskjellige nettverksoperasjoner.

d1_udp.c:

Testkoden oppretter en klient og sender forskjellige data til serveren ved å bruke D1-funksjonene. 
Den venter på bekreftelse fra serveren etter hver dataoverføring. 
Hvis bekreftelsen ikke mottas eller har feil sekvensnummer, 
sendes dataene på nytt. Etter å ha sendt alle dataene, 
avslutter den forbindelsen med serveren. 
Mellom hver mottak er det en ACK-som sendes.

d1_create_client(): Denne funksjonen oppretter en UDP-klient ved å opprette en D1Peer-struktur og tilknytte en UDP-socket til klienten. Den returnerer en peker til D1Peer-strukturen.
d1_delete(): Denne funksjonen frigjør ressursene til en D1Peer-struktur, inkludert lukking av socketen.
d1_get_peer_info(): Denne funksjonen konfigurerer informasjonen til en vert (IP-adresse og portnummer) basert på vertens navn og portnummeret som er oppgitt.
d1_recv_data(): Denne funksjonen mottar data fra en vert ved hjelp av UDP-socketen til klienten. Den validerer også checksummen til de mottatte dataene.
d1_send_data(): Denne funksjonen sender data til en vert ved hjelp av UDP-socketen til klienten. Den beregner også checksummen for å sikre dataintegriteten.
d1_wait_ack(): Denne funksjonen venter på en ACK-pakke fra verten og behandler ACK-pakken ved å sjekke sekvensnummeret og ACK-flagget.
d1_send_ack(): Denne sender en bekreftelsespakke til en peer for å bekrefte mottak av data
calculate_checksum(): Denne beregner en sjekksum for å oppdage overføringsfeil eller datakorrupsjon.


d2_lookup.c:

d2 oppretter en klient og sender en forespørsel til en server for å hente data. 
Den venter på svar fra serveren og behandler svaret ved å lagre dataene lokalt og deretter skrive dem ut. 
Til slutt frigjør den ressursene den har brukt.

d2_client_create(): Denne funksjonen oppretter en D2Client-struktur som inneholder en D1Peer-struktur for kommunikasjon med en server ved å kalle på d1 funksjonen
d2_client_delete(): Denne funksjonen frigjør ressursene til en D2Client-struktur ved å kalle på d1 funksjonen
d2_send_request(): Denne funksjonen sender en forespørselspakke til en server ved hjelp av UDP-kommunikasjon om en request som starter pakkesending fra serveren
d2_recv_response_size(): Denne funksjonen mottar størrelsen på svaret fra serveren.
d2_recv_response(): Denne funksjonen mottar selve svaret fra serveren.
d2_alloc_local_tree(): Denne funksjonen allokerer minne for en lokal trestruktur som brukes til å lagre nettverksnoder.
d2_free_local_tree(): Denne funksjonen frigjør minnet som er allokeret for en lokal trestruktur.
d2_add_to_local_tree(): Denne funksjonen legger til nettverksnoder i den lokale trestrukturen.
d2_print_tree(): Denne funksjonen skriver ut den lokale trestrukturen til konsollen.

