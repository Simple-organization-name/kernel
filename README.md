# kernel

## Special Mappings

| Name          | Flags                   | Page size/type | PML4 | PDPT | PD  | PT | Note                                     |
|---------------|-------------------------|----------------|------|------|-----|----|------------------------------------------|
| pdp_low       | PTE_P / PTE_RW          | PDPT           | 0    |      |     |    |                                          |
| pdp_high      | PTE_P / PTE_RW          | PDPT           | 510  |      |     |    |                                          |
| Identity Map  | PTE_P / PTE_RW / PTE_PS | 1GiB           | 0    | 0    |     |    |                                          |
| kernel        | PTE_P / PTE_RW / PTE_G  | PD             | 510  | 510  |     |    | Should not be touched                    |
| framebuffer   | PTE_P / PTE_RW          | PD             | 510  | 509  |     |    | Should not be touched before init of GPU |
| memManagement | PTE_P / PTE_RW / PTE_NX | PD             | 510  | 508  |     |    |                                          |
| memManagement | PTE_P / PTE_RW / PTE_NX | PD             | 510  | 508  | 510 |    |                                          |
| temp          | PTE_P / PTE_RW / PTE_NX | PT             | 510  | 508  | 511 |    |                                          |
