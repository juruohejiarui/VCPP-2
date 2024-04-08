# Segment Registers
- ``ds``: Data Segment
- ``es``: Extend Segment
- ``cs``: Code Segment
- ``ss``: Stack Segment
- ``fs`` and ``gs``: General-purpose Segment 

In long-mode, ``ds, es, ss, cs`` are treated as their **base** was $0$ no matter what the descriptor is. The limit check is disabled.