# UDP Directory Service Client

## Introduksjon

Dette prosjektet implementerer en enkel og rask katalogtjeneste over UDP (User Datagram Protocol), inspirert av tjenester som Yellow Pages. Prosjektet består av to hoveddeler:

- **Nedre lag**: Håndtering av UDP-kommunikasjon, inkludert sending og mottak av datapakker med ACK-mekanismer.
- **Øvre lag**: Behandling av serverens svar, organisering av data i en trestruktur og utskrift av kataloginformasjon.

Formålet med prosjektet er å lage en effektiv katalogtjeneste som kan håndtere store mengder forespørsler uten behov for vedvarende tilkoblinger. UDP brukes fordi det gir rask og skalerbar kommunikasjon, men samtidig krever det at vi implementerer enkle mekanismer for pakkekvitteringer (ACK) og feilhåndtering. Målet er å strukturere og kontrollere pakkene, sende dem over nettverket, og ekstraktere innholdet som blir sendt.

UDP benyttes for rask og skalerbar kommunikasjon uten vedvarende tilkoblinger. Dette prosjektet inkluderer også mekanismer for feilhåndtering, for eksempel tapte pakker og sekvensering.


## Funksjonalitet

- Sender en forespørsel til serveren med et heltall > 1000, som identifiserer katalogdata som skal hentes.
- Mottar en trestruktur av data, hvor noder har ID-er, verdier og referanser til barnenoder.
- Bruker en enkel protokoll med ACK-pakker for pålitelig overføring over UDP.
- Organiserer og skriver ut katalogtreet basert på serverens respons.
- Håndterer feilsituasjoner som tapte pakker og feil i sekvensering.

## Filstruktur

- `d1_udp.c` – Implementerer UDP-kommunikasjon, inkludert sending, mottak og håndtering av ACK-pakker.
- `d2_lookup.c` – Implementerer trehåndtering og databehandling, inkludert lagring og parsing av serverens svar.
- `d1_udp.h` – Header-fil for UDP-funksjoner og datastrukturer.
- `d2_lookup.h` – Header-fil for trehåndtering og klientkommunikasjon.
- `d1_test_client.c` – Testklient for nedre lag, som validerer UDP-funksjonaliteten.
- `d2_test_client.c` – Testklient for øvre lag, som validerer trehåndtering og datainnhenting.
- `Makefile` – Bygger prosjektet.
- `README.md` – Dokumentasjon.

## Kompilering og kjøring

For å kompilere prosjektet, kjør:

```sh
make

/d1_test_client  # Tester UDP-kommunikasjon
./d2_test_client  # Tester katalogtjenesten

valgrind --leak-check=full --track-origins=yes ./d1_test_client

# Forventet utgang

d1_test_client skal kunne oppdage serveren og sende/motta pakker korrekt.

d2_test_client skal hente katalogtreet og skrive det ut i riktig format.
