# Equipment doll: layers and weapon holders

The doll now provides all seven native clothing layers for the selected body part:
Personal, Close to skin, Normal, Waist, Outer, Strapped and Aura. Empty layers remain
drop targets. Counts and the item list expose multiple pieces on the same layer;
selecting an entry preserves inspection, item actions and Take Off.

Scabbards, Sheaths and Holsters have separate overview slots and individual item
lists. Back uses strapped back coverage and excludes these weapon holders. Slot
classification uses the item's own use action and pocket restrictions, never actions
found recursively inside its contents. This keeps backpacks containing a sheath in Back.

Drag-and-drop keeps the existing Apply/Cancel step. Explicit body/layer targets
reject incompatible items. A weapon dropped onto a compatible equipped holder uses
the native holster operation; dropping onto an individual list entry selects that
exact holder. Native wear limits, encumbrance, storage restrictions and keybindings
remain authoritative.

Validation on a private copied world and X server:

- Client and focused tests compiled successfully.
- 19 assertions passed across classification and native backpack/scabbard coexistence tests.
- Loaded a backpack with two scabbards, two sheaths, a holster and multiple clothing layers.
- Stored a longsword in the selected waist scabbard while leaving the back scabbard and backpack intact.
- Rejected a spear in an incompatible scabbard.
- Rejected an outer jacket on Close to skin; accepted it on Outer and showed both jackets.
- Save inspection confirmed two worn jackets and the longsword inside the chosen baldric.
- The 1280 × 800 and 3440 × 2144 layouts display all seven layer targets; item lists scroll when necessary.
- Reloaded the saved fixture at 3440 × 2144 and confirmed both Outer items and separate Back/Scabbards slots.

Evidence and the staged combined executable are under `artifacts/doll-layers-20260928/`.
The executable includes concurrent achievement/progression work and requires its matching
staged data overlay. It is not a standalone update package. No user installation or save
was modified. Windows runtime verification remains pending.
