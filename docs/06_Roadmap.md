# 📝 Roadmap

Oto lista potencjalnych funkcjonalności, ulepszeń i optymalizacji, które można zaimplementować w silniku **VoxelEngine** w ramach dalszego rozwoju.

---

## 🎮 1. Fizyka i Poruszanie się
Urealnienie zachowania gracza w świecie voxelowym.
- [ ] **Grawitacja i spadanie**: Dodanie stałego przyspieszenia w dół działającego na kamerę gracza, gdy nie jest w trybie niszczenia/budowania.
- [ ] **Kolizje z voxelami**: Wykrywanie kolizji typu AABB (pudełko kolizyjne gracza z blokami) uniemożliwiające przechodzenie przez ściany i ziemię.
- [ ] **Chodzenie i skakanie**: Dodanie obsługi spacji do skoku, gdy gracz dotyka ziemi, oraz ograniczenie lotu kamery (klawisz przełączający tryb latania Spectator vs tryb chodzenia).
- [ ] **Ślizganie i tarcie**: Różne zachowanie w zależności od podłoża (np. lód).

---

## 🎨 2. Ulepszenia Graficzne i Wizualne
Poprawa estetyki renderowania voxelowego świata.
- [ ] **Wokselowe Ambient Occlusion (AO)**: Cieniowanie styków i narożników bloków (przyciemnianie wierzchołków na podstawie obecności sąsiednich voxeli podczas budowania siatki). Nadaje silnikowi głębię 3D.
- [ ] **Dynamiczne oświetlenie**: W pełni dynamiczne źródła światła (np. punktowe/reflektorowe światło przy pochodniach lub graczu) o płynnym zanikaniu intensywności, niezależne od siatki bloków.
- [ ] **Cykl dnia i nocy**: Dynamiczna zmiana koloru tła nieba (clear color) oraz kierunku i barwy światła słonecznego na podstawie czasu systemowego.

---

## 🛠️ 3. Rozgrywka i Edycja Świata
Wzbogacenie interakcji ze światem gry.
- [ ] **Wybór bloków (Hotbar / Pasek wyboru)**:
  - Dodanie paska wyboru bloków w ImGui.
  - Wybór aktywnego bloku za pomocą cyfr `1-4` na klawiaturze.
- [ ] **Dynamiczna fizyka płynów**: W pełni dynamiczny algorytm symulujący naturalne przelewanie i zachowanie płynów (np. modelowanie objętościowe oparte na automatach komórkowych lub uproszczona fizyka cząsteczkowa), pozwalający na realistyczny przepływ wody i wyrównywanie poziomów.
- [ ] **Generator struktur (Drzewa, jaskinie)**:
  - Generowanie jaskiń przy użyciu szumu 3D Perlina/Simplex.
  - Dodawanie prostych struktur, takich jak drzewa (pień z drewna, liście) podczas generowania chunków.

---

## ⚙️ 4. Systemy Podsilnikowe i Optymalizacje
Usprawnienia pod maską.
- [ ] **Zapis ustawień do pliku**: Zapisywanie render distance, czułości myszy i innych opcji do pliku `config.json` lub `.ini`.
- [ ] **Occlusion Culling (GPU)**: Odrzucanie całych chunków, które są całkowicie zasłonięte przez inne chunki znajdujące się bliżej kamery (np. za pomocą Hi-Z lub zapytań o zajętość pikseli).