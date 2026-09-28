# Workstation manager

The HUD Stations button lists adjacent stations. Right-clicking one opens a single Manage entry; Nearby storage also opens the same manager. Its centered window shows contents, processing time and the actions supported by that station. Existing action keys remain available.

Kilns, fireplaces, smoking racks and mills have quantity selection within the manager. Loading filters incompatible materials, limits quantities to available stock and station capacity where applicable, and names the source container or furniture. Nearby vehicle cargo and open container contents are included; sealed pockets are left closed. The mill retains its native crafting pickup range. Transfers and ignition return to the station after their native activity completes. Sealed processing stations cannot be unloaded through the generic transfer control.

Native station handlers continue to own processing, fuel requirements, ignition, stopping, disassembly and output conversion. Fermenting vats, digesters and kegs retain their liquid handling choices. Powered appliances expose their existing control list, including power information, through the same manager. Specialized confirmations, liquid destinations and other native detail prompts remain modal substeps. Generic storage and workbenches expose transfers and crafting; other examinable furniture retains its original controls.

The digester's separate biogas timer now lives on its actual contents rather than dereferencing the end iterator. Collecting gas no longer requires changing the compost's fermentation birthday. The original fuel quantity prompt also checks cancellation and bounds before moving fuel.

## Native checks

Performed on Linux SDL3 at 1280x800, using a private X server and disposable copied world. The player client and saves were not changed by these tests.

- HUD station chooser and right-click management entry; centered compact windows, Close/Back/Escape and native action hotkeys.
- Kiln: compatible fuel filtering; load exactly two logs; unload one; reject ignition without a valid fire source; ignite with a match; return to the active station with a remaining-time display and no load/unload controls. A separately aged fixture completed the cycle and exposed charcoal output, then unloaded exactly 20 charges.
- Smoker: load exactly two food items in the manager; insert exactly 100 charcoal; retain the native missing-fuel gate; load the remaining 1100 from the nearby kiln; ignite; show progress; quench the process. Fuel remains in the rack rather than spilling onto adjacent ground.
- Mill: invalid sheltered windmill reports its location requirement without looping; watermill on flowing water filters for wheat, loads two, starts, stops and removes its contents. Stop and remove also tested using the original hotkeys.
- Digester: loaded liquid fixture exposes add/remove/start controls; starting digestion and reopening progress completes without a crash and displays the fermentation time.
- Fireplace: inline fuel chooser; load two logs; retrieve one through the manager. Cancellation leaves unsubmitted contents unchanged.
- Open crate: load and retrieve one rock; empty storage keeps loading available and disables unloading.
- Local telemetry records station open/close, action choices and transfer item/count. No new network logging.

Screenshots, disposable fixtures and build logs are in `artifacts/workstation-ui-20260927` (excluded from Git).

## Limits

This is representative native interaction coverage, not exhaustive acceptance of every station, appliance, mod, hazard or container state. Complete fermentation/biogas collection, every powered-appliance action and screen-reader operation still need gameplay coverage. Windows was not rebuilt or runtime-tested for this pass. This checkpoint is source work, not a new downloadable release. The shared build also contains concurrent progression changes; packaging should use the finalized combined source.
