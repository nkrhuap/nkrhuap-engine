<div align="center">

  # nkrhuap engine

</div>

**Um motor gráfico e computacional de baixo nível em C++23 e Vulkan 1.4, consumido por um cliente em Rust.**

## Tese
- Layouts contíguos e amigáveis ao cache L1/L2/L3, seguindo os principios de Data-Oriented Design e ademais ideias que circulam a Computação de Alto Desempenho;
- Ausência de vtables no loop de renderização, uso de metaprogramação em C++ e tipos de tamanho estático;
- Assimetria Temporal CPU-GPU, Ownership tipada, RAII de custo zero e Deferred Deletion.

## Arquitetura
1. O Núcleo Gráfico (`nkrhuap engine`): Em C++23, focado em hardware, controle fino de memória e ausência total de código legado, usando o que há de mais moderno no Vulkan 1.4;
2. O Jogo (`salkantkay`): Em Rust, projetado para conter a lógica de gameplay, ciclo de vida e gerenciamento seguro de estado.

### Objetivo a ser atingido
```mermaid
flowchart TD
    subgraph RustFrontend["Rust (salkantkay)"]
        Scene["Game State / Scene / ECS"]
        Handle["MeshHandle (Possui ID + impl Drop)"]
        Batch["Array Contíguo de Instâncias<br/>[InstanceData; N]"]
        DropCall["Drop::drop() -> Dispara FFI Release"]
    end

    subgraph FFI["Fronteira C ABI (nkrhuap_api.hpp)"]
        F1["nkrhuap_load_mesh(data) -> uint32_t ID"]
        F2["nkrhuap_release_mesh(uint32_t ID)"]
        F3["nkrhuap_submit_frame(*const InstanceData, count)"]
    end

    subgraph CPPEngine["C++23 Core (nkrhuap-engine)"]
        SlotMap["Catálogo de Buffers (Dense SlotMap)<br/>ID -> VkBuffer + VmaAllocation"]
        Timeline["Vulkan 1.4 Timeline Semaphore<br/>(Marca progresso do hardware)"]
        DeletionQueue["Deferred Deletion Queue<br/>(Buffer liberado quando Semaphore >= frame)"]
        VkRender["Dynamic Rendering Pipeline<br/>(Lê buffers direto da VRAM)"]
    end

    Handle -->|Alocação inicial| F1
    F1 -->|Aloca VMA e registra no índice| SlotMap
    
    Handle -.->|Ao sair de escopo| DropCall
    DropCall --> F2
    F2 -->|Agenda liberação segura| DeletionQueue
    Timeline -->|Valida frame concluído| DeletionQueue
    DeletionQueue -->|vmaDestroyBuffer físico| SlotMap

    Scene -->|Gera instâncias| Batch
    Batch -->|Zero-Copy / Slice Pointer| F3
    F3 -->|Resolve IDs no SlotMap| VkRender
```

## Build e Run
**Pré-requisitos:**
- Clang
- C++23
- Rust toolchain
- CMake and Ninja
- Vulkan SDK (v1.14)

```bash
cd salkantkay/
cargo run
```
