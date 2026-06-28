# 🧪 Koncepcja Hybrydowego Silnika Wokselowego z Fizyką

Ta notatka opisuje propozycję rozszerzenia silnika **[[api/VoxelEngine|VoxelEngine]]** do postaci **silnika hybrydowego**. Łączy on zalety dynamicznego terenu opartego o siatkę wokseli z wydajnym renderowaniem wolnostojących, szczegółowych modeli 3D (flory, budynków), a także zintegrowaną fizyką ciał sztywnych i sypkich materiałów.

---

## 🏗️ Proponowany Diagram Architektury Hybrydowej

Poniższy diagram klas przedstawia nowo dodane komponenty i ich integrację z istniejącymi klasami silnika:

```mermaid
classDiagram
    class VoxelEngine {
        -std::unique_ptr~Window~ m_window
        -std::unique_ptr~Input~ m_input
        -std::unique_ptr~Camera~ m_camera
        -std::unique_ptr~World~ m_world
        -std::unique_ptr~PhysicsSystem~ m_physicsSystem
        -std::unique_ptr~AssetManager~ m_assetManager
        -VulkanContext m_vulkanContext
        +run()
    }
    class World {
        -std::unordered_map~ChunkCoord, unique_ptr~Chunk~~ m_chunkMap
        -std::vector~unique_ptr~Entity~~ m_entities
        -std::unique_ptr~VoxelParticleSystem~ m_particleSystem
        +update(deltaTime)
        +digAt(rayStart, rayDir)
    }
    class Chunk {
        -Block m_blocks[64][64][64]
        -bool m_isDirty
        +setBlock(x, y, z, block)
        +getBlock(x, y, z) Block
        +buildMesh()
    }
    class PhysicsSystem {
        +update(deltaTime, world)
        +resolveCollisions(entity, chunk)
        +checkRaycast(start, dir, outBlockPos) bool
    }
    class AssetManager {
        -std::unordered_map~string, MeshData~ m_loadedMeshes
        +loadVoxelMesh(filepath) Mesh*
    }
    class Entity {
        -glm::vec3 m_position
        -glm::vec3 m_rotation
        -Mesh* m_mesh
        -PhysicsBody m_physicsBody
        +update(deltaTime)
    }
    class VoxelParticleSystem {
        -std::vector~Particle~ m_particles
        +spawnSandParticle(pos, velocity)
        +update(deltaTime, world, physics)
    }
    class VulkanContext {
        -std::unique_ptr~MegaBuffer~ m_megaVertexBuffer
        +drawFrame(viewMatrix, chunks, entities)
        -drawChunks()
        -drawEntities()
    }

    VoxelEngine --> World : Zarządza światem i obiektami
    VoxelEngine --> PhysicsSystem : Aktualizuje fizykę
    VoxelEngine --> AssetManager : Odpowiada za wczytywanie modeli
    World --> Chunk : Zarządza wokselami terenu
    World --> Entity : Zarządza obiektami (flora, budynki)
    World --> VoxelParticleSystem : Zarządza sypkim piaskiem/ziemią
    VoxelParticleSystem --> PhysicsSystem : Korzysta z detekcji kolizji
    Entity --> AssetManager : Pobiera siatkę geometryczną
    VulkanContext --> World : Pobiera dane chunków i encji do narysowania
```

---

## 🛠️ Opis Nowych Komponentów

### 1. `AssetManager`
Klasa odpowiedzialna za ładowanie i buforowanie zoptymalizowanych siatek wokselowych (np. wyeksportowanych z MagicaVoxel do formatów `.obj` lub `.gltf`). Zapewnia, że dany model (np. model drzewa o wymiarach 16x16x32 wokseli) jest ładowany do pamięci VRAM tylko raz, niezależnie od liczby jego instancji na scenie.

### 2. `Entity` (Obiekt Gry)
Reprezentuje dowolny niezależny obiekt 3D umieszczony na świecie o precyzyjnych współrzędnych zmiennoprzecinkowych (`glm::vec3`). Każdy obiekt może reprezentować budynek, florę lub pojazd. 
* Posiada wskaźnik do siatki geometrycznej w `AssetManager`.
* Może opcjonalnie posiadać komponent `PhysicsBody`, pozwalający silnikowi fizyki na symulację kolizji, grawitacji i pędu.

### 3. `PhysicsSystem`
Obsługuje detekcję i rozwiązywanie kolizji obiektów dynamicznych z terenem wokselowym oraz innymi encjami.
* **Kolizja AABB z wokselami:** Obliczanie kolizji jest wysoce zoptymalizowane – zamiast testować trójkąty terenu, silnik sprawdza pozycję bounding boxa encji bezpośrednio w strukturze siatki [[api/Chunk|Chunk]].
* **Integracja:** Może bazować na własnej implementacji uproszczonych kolizji lub integrować zewnętrzną bibliotekę (np. Jolt Physics).

### 4. `VoxelParticleSystem`
Zarządza dynamicznymi cząsteczkami fizycznymi generowanymi podczas zniekształcania terenu (kopanie, kruszenie, osypywanie piasku/ziemi).

---

## 🔄 Przepływ Fizyki Sypkich Materiałów (Kopanie i Przesypywanie)

Poniższy schemat przedstawia cykl życia sypkiego bloku (np. piasku) podczas interakcji z łopatą gracza lub z powodu grawitacji:

```mermaid
flowchart TD
    Start[Gracz kopie ziemię/piasek łopatą] --> Raycast[Raycast wykrywa woksel uderzenia]
    Raycast --> CheckType{Czy to blok sypki?\nnp. Sand, Gravel}
    
    CheckType -- Nie (np. Stone) --> Destroy[Usuń woksel i dodaj efekt pyłu]
    
    CheckType -- Tak --> SpawnParticles[1. Usuń woksel z Chunku\n2. Zespawnuj kilka cząsteczek fizycznych w tym miejscu]
    
    SpawnParticles --> PhysUpdate[Fizyka symuluje ruch cząsteczek]
    PhysUpdate --> Collision{Czy cząsteczka uderzyła o grunt?}
    
    Collision -- Nie --> PhysUpdate
    
    Collision -- Tak i zatrzymała się --> Settle[Oblicz najbliższą wolną pozycję woksela w siatce]
    Settle --> RecreateVoxel[1. Usuń cząsteczkę\n2. Wstaw woksel z powrotem do Chunku]
    RecreateVoxel --> RebuildMesh[Ustaw Chunk::setDirty i przebuduj siatkę w tle]
    RebuildMesh --> End([Koniec cyklu])
```

---

## 💾 Wpływ na Renderowanie (VulkanContext)

Aby obsłużyć tę strukturę w potoku graficznym Vulkan:
1. **Renderowanie terenu:** Siatka terenu wciąż jest generowana za pomocą *Greedy Meshing* w klasach [[api/Chunk|Chunk]] i przesyłana do [[api/MegaBuffer|MegaBuffer]].
2. **Renderowanie obiektów (Instancing):** Wolnostojące obiekty (drzewa, skrzynie) są renderowane w osobnym przebiegu (Render Pass / Dynamic Rendering). Zamiast wysyłać pojedyncze wywołania rysowania dla każdego drzewa, stosujemy **GPU Instancing** – przekazujemy tablicę macierzy transformacji wszystkich instancji tego samego modelu i rysujemy je jednym wywołaniem `vkCmdDrawIndexedInstanced`.
3. **Renderowanie cząsteczek piasku:** Cząsteczki są renderowane jako małe instancjonowane sześciany lub billboardy, co gwarantuje płynność nawet przy tysiącach ziaren na ekranie.
