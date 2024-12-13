# Zadanie 2

---

Kod realizujący github actions: [ghcr_action.yml](.github/workflows/ghcr_action.yml)

Opis kroków:
- checkout
- metadane
- QEMU (dodatkowe architektury budowania obrazu)
- Buildx (narzędzie budowania)
- Logowanie do usługi Docker (wymagane dla docker scout)
- Logowanie do usługi Github (wymagane do nadpisania obrazu w rejestrze ghcr.io)
- Budowanie testowe
- Sprawdzenie obrazu za pomocą docker scout
- Budowanie wraz z umieszczeniem obrazu na prywatnym i publicznym repozytorium

Efekt działania gha ([wykonanie](https://github.com/gmplk/LWServer/actions/runs/12320518279)):
![cves_gha](scr/cves.png)
