# Reduce DMA configuration sourced from DT to minimum needed
## Description (additional details on context & rationale)

Drivers which use DMAs configure cells of the dmas Devicetree property to
initialize the Zephyr `struct dma_config`.  However, many of the fields in the
DT are redundant/useless because the driver knows in advance what they should
be.

For example:
------------

 - Transfer direction (RX DMA is always *P2M*, TX DMA is always *M2P*)
 - Address increment (always `MEM_INC | PERIPH_NO_INC`?)
 - Data size, depending on driver
 -
    - Some drivers are hardcoded to use 8-bit *on purpose*.
    - Some drivers accept configuration from DTS; should not be removed.

## Definition of Done (bounds of the task)

- All drivers directly initialize known-in-advance fields of `struct
  dma_config` directly instead of using the values found in Devicetree
- All board DTS and overlays is cleaned up to set only the necessary flags

