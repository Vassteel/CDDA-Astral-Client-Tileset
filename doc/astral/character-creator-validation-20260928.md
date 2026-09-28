# Character creation window validation

Character creation now embeds its selection list beside the details in one centered window. List and detail scrolling are independent; category tabs and original input callbacks remain in use. Equipment headers render color markup, while selectable equipment names remove the markup before passing text to ImGui. Cancel, Previous, Next and Finish buttons use the existing actions and confirmations.

## Checks

- Linux SDL3 compilation and linking passed.
- Reproduced the separate list window and literal equipment color tags in the preceding build.
- Opened all eight tabs at 1280x800 in a disposable profile on a private X server.
- Verified mouse tab navigation, equipment categories, keyboard list selection, equipment item actions, empty search results and clearing the filter.
- Confirmed equipment condition and water-content tags no longer print literally; condition colors still render in the list and details header.
- Finish opens the existing confirmation; cancelling it keeps the editor open. Cancel returns to the main menu without leaving a detached list.
- Checked the centered window at 3440x2144, including General Info collapsed.

The staged executable, hashes, build logs and screenshots are under `artifacts/character-creator-ui-20260928`. The running installation was not modified. This is a shared-checkout development build containing concurrent progression changes, not a standalone release package. Windows and screen-reader acceptance were not tested in this pass.

A separate observation remains for follow-up: resizing during creation can leave the main-menu backdrop partially painted until re-entering the menu; starting at the target resolution paints it correctly. The creation window itself stays centered.
