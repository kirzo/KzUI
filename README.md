<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/KzUI_banner.jpg" alt="KzUI Banner" width="512">
</p>

<h1 align="center">KzUI</h1>

<p align="center">
  <em>A lightweight UMG framework for Unreal Engine: semantic input per local player, gamepad navigation, themed button prompts, and ready-made kits for settings, splash screens and modal prompts.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.x-blue?logo=unrealengine" alt="Unreal Engine 5.x" />
  <img src="https://img.shields.io/badge/language-C%2B%2B20-00599C?logo=c%2B%2B" alt="C++20" />
  <img src="https://img.shields.io/badge/Blueprint-ready-orange" alt="Blueprint ready" />
  <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="MIT License" />
  <img src="https://img.shields.io/github/stars/kirzo/KzUI?style=social" alt="GitHub stars" />
</p>

---

## Table of Contents

- [Overview](#overview)
- [Philosophy](#philosophy)
- [Modules](#modules)
- [Feature Tour](#feature-tour)
- [Requirements](#requirements)
- [Installation](#installation)
- [Tutorials](#tutorials)
  - [1. Project Setup](#1-project-setup)
  - [2. Your First Navigable Menu](#2-your-first-navigable-menu)
  - [3. Reacting to Options: Switch on Option](#3-reacting-to-options-switch-on-option)
  - [4. Text Styles and Widget States](#4-text-styles-and-widget-states)
  - [5. Button Prompts with Inline Icons](#5-button-prompts-with-inline-icons)
  - [6. Icon Variations and Hold Progress](#6-icon-variations-and-hold-progress)
  - [7. Sounds](#7-sounds)
  - [8. Input Triggers: Hold, Double Tap and Chords](#8-input-triggers-hold-double-tap-and-chords)
  - [9. Sub-Screens and Modals: Dynamic Children](#9-sub-screens-and-modals-dynamic-children)
  - [10. Modal Prompts](#10-modal-prompts)
  - [11. Screen Paging with KzWidgetSwitcher](#11-screen-paging-with-kzwidgetswitcher)
  - [12. A Complete Settings Screen](#12-a-complete-settings-screen)
  - [13. Splash Screens](#13-splash-screens)
  - [14. Local Multiplayer](#14-local-multiplayer)
- [Notes and Caveats](#notes-and-caveats)
- [Asset Naming](#asset-naming)
- [Repository Layout](#repository-layout)
- [Related Projects](#related-projects)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

---

## Overview

**KzUI** is an open-source UMG plugin for **Unreal Engine 5** that replaces raw key handling with **semantic UI inputs** (Accept, Back, navigation, and friends), mapped per platform and resolved per local player. On top of that input core it provides gamepad-navigable widgets with **geometric navigation** (no per-widget wiring), a **class-based text style system**, **button prompt icons** that follow the active input device, sound and icon **themes**, and ready-made kits for the screens every game rebuilds from scratch: **settings**, **splash sequences**, and **modal prompts**.

Everything is driven from C++ base classes and project settings: your game supplies the Widget Blueprints and the art, KzUI supplies the behavior.

---

## Philosophy

A few deliberate design decisions shape the whole plugin; knowing them makes everything else predictable:

- **Semantic inputs, not keys.** Widgets react to *Accept* or *Back*, never to `Gamepad_FaceButton_Bottom`. The key mapping lives once in project settings, with per-platform overrides.
- **One screen owns the focus.** A screen (`KzUserWidget`) keeps the user focus and manages an internal *hover* cursor between its options. Focus does not jump between buttons, which keeps local multiplayer routing sane.
- **Unhandled input bubbles.** A widget only consumes an input it actually acted on. Anything else falls through: to a parent screen first, then to the game. Dual-use buttons (UI *and* gameplay) work out of the box.
- **Hover is the cursor, Selected is the commit.** Navigation moves the hover; Accept commits the hovered option as the selection, which persists (a menu remembers its selected entry when you come back to it).
- **Per player everything.** Widget stack, focus policy, input device, themes: all resolved through a `ULocalPlayerSubsystem`, so four players with four gamepads get four independent UIs.
- **Plug and play.** Sensible automagic defaults (auto-collected options, auto-bound settings rows, auto-refreshed prompt icons) with explicit override points everywhere.

---

## Modules

| Module            | Type         | Purpose                                                                                     |
|-------------------|--------------|---------------------------------------------------------------------------------------------|
| **`KzUI`**        | Runtime      | Everything: input core, widgets, themes, kits, subsystem, settings.                         |
| **`KzUIUncooked`**| UncookedOnly | Custom K2 nodes (`Switch on Option`).                                                       |
| **`KzUIEditor`**  | Editor       | Asset factories (themes, flipbooks) and details customizations (Switch on Option toggles).  |

---

## Feature Tour

### Input core

- **`EKzUIInputType`**: the semantic inputs `Back`, `Accept`, `Down`, `Up`, `Left`, `Right`, `Previous`, `Next`, `Start`, `Select`.
- **`UKzUIInputSettings`**: project-wide key map per input, with per-platform overrides (Windows / Xbox / PlayStation / Switch) in `Override`, `Additive` or `Subtractive` mode, a trigger config per input, device-name mappings, cursor policy, default themes and prompt class.
- **Input triggers**: `Press`, `DoubleTap`, `Chord` (Enhanced-Input-style: required inputs held first), `Hold` (with `GetInputHoldProgress` for progress bars).
- **`UKzUIInputSubsystem`** (per local player): widget stack, focus restore policy, input suspension (`SuspendInput` / solo-input screens), input device tracking with change broadcasts, per-player theme overrides.
- **`FKzUIStickKeyProcessor`**: normalizes analog sticks into digital `Gamepad_*Stick_*` keys with hysteresis and key-repeat for **every** gamepad backend, including pads that GameInput treats as generic controllers and that never send those keys natively.

### Widgets

- **`UKzUserWidget`**: the screen base class, with semantic input events (`Triggered` / `Held` / `Released` per input, plus `CanHandleX` gates), input masks, per-widget sounds, a single-slot modal child system (`CreateDynamicChild`, `bHideWhileOpen`), `ShowPrompt`, and screen activation events.
- **`UKzNavigableWidget`**: navigable menu with options collected automatically from the tree (`GetOptions` / `GetOptionsUnder`), **geometric navigation** in any direction with designer-rule overrides, hover/selection state with sounds, edge events, loop option, and option-row helpers (`RevertOptions`, `HasChangedOptions`).
- **`IKzSelectableWidgetInterface`**: what makes a widget an option, providing hover/selection notifications, linked selectables (style propagation to inner widgets), `CanBeSelected` (hover-only rows), and input claiming (`WantsInput` / `HandleInput`) so a hovered row can consume Left/Right for itself.
- **`UKzTextBlock`**: TextBlock with per-state class-based styles and inline input icons parsed from tokens (`"Press {Accept} to join"`), per-word auto wrap, justification-aware spacing, auto icon sizing.
- **`UKzImage` / `UKzInputIcon` / `UKzUIFlipbook`**: image with soft-texture streaming helpers, flipbook playback synced to a global clock, dynamic material parameter access; the input icon resolves itself from the icon theme and refreshes on device changes.

### Themes and styles

- **`UKzTextStyle` / `UKzTextBlockStyle`**: one look (font, color, shadow) as a Blueprint-subclassable class; a style set maps looks to widget states (`Normal` / `Hovered` / `Selected` / `Disabled`) with per-widget state overrides.
- **`UKzUISoundTheme`**: per-input sounds plus `HoverSound` and `SelectSound`.
- **`UKzUIIconTheme`**: button prompt icons per input and device (texture, material or flipbook), project-defined custom tokens, and **icon variations**: materials that wrap the device-resolved texture (`{Back:Hold}`) so one material serves every platform.

### Kits

- **Settings**: `UKzSpinner`, `UKzSlider`, `UKzToggle` option rows (look-free, `BindWidgetOptional` parts) with per-row snapshot/revert, deferred-apply mode, and **auto-binding to engine settings** (resolution, window mode, quality presets and individual scalability groups, VSync, resolution scale) through `UKzSettingsLibrary`.
- **Splash**: `UKzSplashWidget` plays `UKzSplashPage` containers (any content, per-page timing/sound/skippability) as a fade sequence, swallowing all input while alive; any button on any device skips.
- **Prompts**: `UKzPromptWidget` confirm/cancel modal with smart Back handling, spawned in one call with `ShowPrompt` using the project-settings prompt class.
- **Paging**: `UKzWidgetSwitcher` + `UKzSwitcherPage`, switcher pages that plug into the input model (a screen used as a page registers/focuses on activation and its Back returns to the previous page), with per-page titles and change events.

### Blueprint tooling

- **`Switch on Option`** (K2 node): one exec pin per selectable widget of the Blueprint, enumerated from the widget tree. Renames orphan the pin visibly instead of failing silently; pins can be toggled off per node in the details panel.

---

## Requirements

- **Unreal Engine 5.x** with a C++-enabled project.
- **[KzLib](https://github.com/kirzo/KzLib)**: KzUI depends on the KzLib plugin (editor infrastructure).

---

## Installation

```bash
cd <YourProject>/Plugins
git clone https://github.com/kirzo/KzLib.git
git clone https://github.com/kirzo/KzUI.git
```

1. Right-click your `.uproject` → **Generate Visual Studio project files**.
2. Build and launch; enable via **Edit → Plugins → KzUI** if needed.
3. To use the C++ types from your game module, add `"KzUI"` to your `*.Build.cs` dependencies. Pure Blueprint usage needs no module changes.

---

## Tutorials

The tutorials build on each other and mirror how a real game front-end comes together: input setup, a menu, styles and prompts, then the full settings/splash kits.

### 1. Project Setup

Everything global lives in **Project Settings → Plugins → KzUI**:

1. **Input Map**: every semantic input ships with gamepad defaults (Accept = bottom face button, Back = right face button, navigation = left stick + dpad, Previous/Next = shoulders, Start/Select = specials) and additive keyboard keys on desktop (WASD/arrows, Space, BackSpace, Enter, Tab). Adjust keys per input, per platform, in `Override`/`Additive`/`Subtractive` mode. The Switch entries demonstrate swapped Accept/Back.
2. **Device Mappings**: hardware device name → icon family (`DualSense`, `DualShock`, `Xbox`, `Keyboard`...). The defaults cover XInput, WinDualShock and the common GameInput identifiers; add your own if your platform reports different names.
3. **Cursor Policy**: `Manual` (KzUI never touches the mouse cursor) or `ByDevice` (cursor visible only while the player uses keyboard/mouse).
4. **Sound Theme / Icon Theme / Prompt Class**: assign once; later tutorials create them.

> Per-platform key overrides resolve by the running platform, so PS/Xbox/Switch branches only take effect in cooked builds for those platforms.

### 2. Your First Navigable Menu

Goal: a pause menu navigable with dpad or left stick, mouse included, in ~5 minutes.

1. Create `WBP_PauseMenu` with parent class **`KzNavigableWidget`**.
2. Add a `VerticalBox` and drop three **`KzTextBlock`** widgets in it: `Resume`, `Settings`, `Quit`. On each one, set **`Selectable = true`** (that is what turns a text into an option).
3. Class Defaults: check **`bStartWithInputEnabled`**. If this menu should own the whole screen in local multiplayer, also check **`bSoloInput`**: every other player's UI input is suspended while it lives.
4. Open it from gameplay: `Create Widget` → `Add to Viewport` (or `Add to Player Screen`). The widget registers itself on its player's stack and takes focus; no extra wiring.
5. Play: Up/Down moves the hover (geometrically, no navigation links to author), Accept commits, mouse hover and click work too. Back does nothing yet; add `On Back Input Triggered` → `Remove from Parent`.

How options are found: the default `GetOptions` walks the widget tree and collects every widget implementing `IKzSelectableWidgetInterface` with `IsSelectable` on, in tree order, including your own UserWidget entries (implement the interface in their Blueprint; remember the auto-created `Is Selectable` stub returns **false** until you make it return true). To scope collection to one container, override `GetOptions` → `GetOptionsUnder(MyVerticalBox)`.

Navigation fine print, when you need it:

- Designer **Navigation rules** on a widget win over the geometric search, and a `Stop` rule is a hard wall.
- `bLoopNavigation` wraps around the edges; without it, `On Navigation Edge` fires when a direction has nowhere to go.
- `bPreviousNextNavigation` makes the shoulder inputs step through options in tree order (off by default).
- Hidden or disabled widgets are skipped and navigated *through*.

### 3. Reacting to Options: Switch on Option

Comparing option names by hand is fragile. The **Switch on Option** node (right-click menu, category *Kz UI*, only inside `KzNavigableWidget` Blueprints) generates **one exec pin per selectable widget** of your tree, plus an explicit `Option` input pin:

1. Add event **`On Option Accepted`**.
2. Add **Switch on Option** and connect the `Option` output into its input pin.
3. Wire each named pin (`Resume`, `Settings`, `Quit`) to its logic.

Pins refresh when the Blueprint compiles. If you rename a widget, its pin becomes a visible orphan (with the wire preserved) instead of silently misrouting. In the node's details panel, a checkbox per option lets you disable pins you do not care about in a particular switch.

### 4. Text Styles and Widget States

Styles are Blueprint **classes**, so they cascade through inheritance:

1. Create a Blueprint of **`KzTextStyle`** (e.g. `TS_Menu`): font, color, shadow. Subclass it for the hovered look (`TS_Menu_Hovered`) changing only the color.
2. Create a Blueprint of **`KzTextBlockStyle`** (e.g. `TSS_Menu`) and fill the state slots: `Normal`, `Hovered`, optionally `Selected` and `Disabled`. Empty slots fall back to `Normal`.
3. Assign `TSS_Menu` to the `Style` property of your option KzTextBlocks.

State resolution priority is `Disabled > Hovered > Selected > Normal`: the hover always shows while the cursor is on top, the selected look shows when the cursor is elsewhere. Per-instance `StateOverrides` swap individual states on one widget without a new style set.

States propagate: a row widget (a UserWidget option) forwards its hover/selection to the widgets returned by `GetLinkedSelectables`, so its inner texts light up with it, regardless of their own `Selectable` flag.

### 5. Button Prompts with Inline Icons

1. Create an **Icon Theme** asset (Content Browser → *KzLib → UI → UI Icon Theme*, e.g. `IT_Default`). For each input, set a `Default` icon and per-device `Overrides` (Xbox / DualShock / DualSense / Switch / Keyboard). Icons can be textures, materials, or **flipbooks** for animated prompts.
2. Assign it in Project Settings → KzUI → Icon Theme.
3. In any **`KzTextBlock`**, just write tokens in the text:

   ```
   Press {Accept} to Join
   Hold {Back} to Leave
   ```

   Tokens become inline `KzInputIcon` widgets sized to the line height, wrapping correctly with the text, and **swapping automatically when the player's device changes** (plug a DualSense mid-game and the icons follow).
4. **Custom tokens**: add entries to the theme's `CustomIcons` map (`MoveTutorial` → per-device images) and use `{MoveTutorial}` anywhere. Unknown tokens render as literal text.
5. Runtime access: `GetIcon(Input)` / `GetCustomIcon(Token)` on the text block return the generated icon widgets (do not cache them across `SetText`, they are regenerated).

Standalone icons: drop a **`KzInputIcon`** widget directly (set `Input` or `CustomToken`), with a `PreviewDevice` property for designer preview.

### 6. Icon Variations and Hold Progress

A *variation* presents the same icon differently; the canonical case is a radial fill while holding a button. One material serves every device, because the theme injects the device-resolved texture at runtime:

1. Author a material (UI domain) with a texture parameter named `Icon` (configurable via the theme's `VariationTextureParameter`), a `Progress` scalar, and your fill effect. Pack shape masks into the icon texture's channels if the effect needs them.
2. Register it in the icon theme's **`IconVariations`** map under a name, e.g. `Hold`.
3. Use it with the extended token syntax:

   ```
   Hold {Back:Hold} to Leave
   ```

4. Drive it from the hold events (see the next tutorial for the Hold trigger):

   ```
   On Back Input Held  → GetIcon(Back) → SetMaterialScalarParameter("Progress", GetInputHoldProgress(Back))
   On Back Input Released → GetIcon(Back) → SetMaterialScalarParameter("Progress", 0.0)
   ```

The dynamic material instance persists across device changes (only the texture parameter moves), so a hold in progress survives a controller swap. Each icon widget owns its instance: four players get four independent fills from one material.

### 7. Sounds

1. Create a **Sound Theme** asset (`ST_Default`) and assign it in the settings.
2. Three layers of feedback, by intent:
   - **`HoverSound`**: the navigation tick (hover moves, spinner/slider steps).
   - **`SelectSound`**: the commit (Accept on an option, toggling a toggle, confirming a prompt).
   - **`InputSounds`** map: raw per-input feedback for everything else. `Back` is the usual entry (close/cancel). Leave `Accept` empty if you use `SelectSound`, or menus will double-fire.
3. Per-widget overrides: `HoverSoundOverride` / `SelectSoundOverride` on navigables, `InputSoundOverrides` map on any screen.

Sounds only play for inputs the widget actually handled (`bPlaySoundOnlyHandled`), and action feedback is gated on effect: stepping a spinner already at its end stays silent.

### 8. Input Triggers: Hold, Double Tap and Chords

Each input has a trigger in the settings (shared by all platforms), overridable per widget with `OverrideInputTrigger`:

- **`Press`**: fires on key down. The default.
- **`Hold`**: fires once after `HoldTime` seconds. The press is *not* consumed, so the same physical button keeps working for the game until the hold completes; releasing early cancels (and `Released` still fires so you can undo feedback animations). `GetInputHoldProgress` returns 0..1 for progress visuals.
- **`DoubleTap`**: fires on the second press within `TapInterval`. First taps bubble to the game.
- **`Chord`**: fires on its keys only while every `RequiredInputs` entry is already held (modifier-first, like Enhanced Input chords). Incomplete chords bubble.

This is the machinery behind dual-use buttons: a "Hold B to leave" lobby widget coexists with B dropping the axe in gameplay, because only the *completed* hold consumes anything.

### 9. Sub-Screens and Modals: Dynamic Children

A screen can open exactly one child screen at a time; while the child lives, the parent's input is suspended automatically:

- **`CreateDynamicChild(Class, ZOrder, bHideWhileOpen)`**: creates the child and adds it to the viewport. With `bHideWhileOpen`, the parent collapses and restores its exact previous visibility when the child closes.
- **`CreateSlottedDynamicChild(Panel, Class)`**: same, but places the child *inside* one of the parent's panels (a persistent frame around swappable content).
- The child closes itself with **`RemoveFromStack`**; the parent gets focus back and `On Child Removed` fires.
- To react to a *specific* child closing, bind to the instance: `CreateDynamicChild` → **Bind Event to On Removed From Parent**. Identification by binding, not by name comparison.

### 10. Modal Prompts

1. Create `WBP_Prompt` with parent class **`KzPromptWidget`**. Tree: a full-screen dimmer, a panel, and three **`KzTextBlock`** widgets named exactly **`MessageText`** (required), **`ConfirmText`** and **`CancelText`** (optional). Give the last two a style with a `Hovered` state; the prompt makes them selectable by itself.
2. Graph: `On Confirm` → `RemoveFromStack`, `On Cancel` → `RemoveFromStack` (play your exit animation in between if you have one; the base class fires the callbacks and disables input, but closing is yours).
3. Assign it in Project Settings → KzUI → **Prompt Class**.
4. Open it from any screen with a single node:

   ```
   ShowPrompt("Apply changes?", "Apply", "Discard", OnConfirmEvent, OnCancelEvent)
   ```

   The callback pins are plain event pins (drag → Create Event). An advanced `ClassOverride` pin swaps the prompt class for special cases.

Built-in behavior: hover starts on Confirm; Left/Right navigates; Accept executes the hovered button; **Back on Confirm moves the hover to Cancel** instead of dismissing (an impulsive Back never discards invisibly), Back on Cancel cancels. Confirm sounds as `SelectSound`, Cancel as the `Back` input sound, whatever the physical key.

### 11. Screen Paging with KzWidgetSwitcher

For a framed menu where sub-screens replace the content but the frame and title persist:

1. In `WBP_PauseMenu`, build: background frame + a title `KzTextBlock` + a **`KzWidgetSwitcher`**.
2. Fill the switcher with **`KzSwitcherPage`** containers (palette, *Kz UI*), each with its `Title` set:
   - Page 0: `KzSwitcherPage("Pause Menu")` containing your options VerticalBox.
   - Page 1: `KzSwitcherPage("Settings")` containing an instance of `WBP_Settings`.
3. Open a page from the Switch on Option: `PageSwitcher → Set Active Widget Index (1)`.
4. Title: bind **`On Active Page Changed`** → `TitleText.SetText(Title)`. The event also announces the initial page after construct, so one binding covers everything.

What the switcher does under the hood: a `KzUserWidget` page is registered on the player's stack when activated (taking focus and silencing the owning screen) and unregistered when it leaves; calling `RemoveFromStack` on it (its normal Back handling) returns the switcher to the previous page instead of destroying anything. **The same `WBP_Settings` works unchanged as a standalone screen or as a page.**

Contract: the default page must be plain content (not a `KzUserWidget`), and screen pages go as direct children of the switcher or wrapped in a `KzSwitcherPage`.

### 12. A Complete Settings Screen

The settings kit splits the problem in three: option **rows** (interaction), a **library** (engine settings plumbing), and your **screen** (assembly).

**Rows**: create one Widget Blueprint per row type, reusable across the whole game:

1. `WBP_SpinnerRow` (parent **`KzSpinner`**): a label, a `KzTextBlock` named exactly **`ValueText`**, optional arrows. While hovered, Left/Right cycles the values; `On Stepped (bForward)` is the hook for arrow pulse animations.
2. `WBP_SliderRow` (parent **`KzSlider`**): a `ProgressBar` named **`Bar`** and/or a `ValueText` (percent). Left/Right adjusts by `Step`.
3. `WBP_ToggleRow` (parent **`KzToggle`**): `ValueText` (On/Off texts) and/or an `Image` named **`ValueImage`** (checked/unchecked brushes). Accept flips it; for Left/Right semantics use a two-option spinner instead.

Rows are hover-only options (`CanBeSelected` = false): Accept never "selects" them, they act.

**Auto-binding**: set the row's `Setting` property and it populates, applies and saves by itself:

| Setting            | Row      | Notes                                                        |
|--------------------|----------|--------------------------------------------------------------|
| `Resolution`       | Spinner  | Supported fullscreen resolutions.                            |
| `WindowMode`       | Spinner  | Fullscreen / Borderless / Windowed.                          |
| `Quality`          | Spinner  | `QualitySetting` picks Overall or one scalability group (Shadows, Textures...). Mixed levels display as `CustomText` until the first step. |
| `VSync`            | Toggle or 2-option Spinner. |                                           |
| `ResolutionScale`  | Slider   |                                                              |

Custom settings (volumes, gameplay flags): leave `Setting = None` and wire `On Value Changed` to your systems; persistence is yours.

**The screen**: `WBP_Settings` (parent `KzNavigableWidget`):

1. A VerticalBox with row instances. Up/Down navigates rows, Left/Right adjusts the hovered one: the row *claims* those inputs through the selectable interface.
2. Instant-apply flow: done. Rows with `bApplyImmediately` (default) apply and save on every change.
3. Confirm-on-exit flow: set `bApplyImmediately = false` on the rows you want gated, then:

   ```
   On Back Input Triggered
     → HasChangedOptions?
        false → RemoveFromStack
        true  → ShowPrompt("Apply changes?", "Apply", "Discard", Confirmed, Discarded)
   Confirmed → KzSettingsLibrary::ApplySettings → RemoveFromStack
   Discarded → RevertOptions → RemoveFromStack
   ```

   `RevertOptions` restores every row to its value at screen activation *through its own setter*, so displays and custom `On Value Changed` wiring update too. Rows re-read their values and re-baseline on every screen activation (`Resync`), which keeps switcher-page settings honest across visits.

### 13. Splash Screens

1. Create `WBP_Splash` (parent **`KzSplashWidget`**): a black background and a `WidgetSwitcher` named exactly **`Pages`**.
2. Fill it with **`KzSplashPage`** containers, each wrapping any content (logo image, legal text, an animated widget) and carrying its own `Duration`, `FadeInTime` / `FadeOutTime`, `bSkippable` and `Sound`. Preview pages in the designer by changing the switcher's active index.
3. Bind **`On Finished`** → whatever comes next (`Remove from Parent`, open a level...). The widget decides nothing on its own.
4. Recommended setup: no boot map; create the splash **on top of your first level** (Level Blueprint or GameMode BeginPlay, `Add to Viewport` with a high Z-order). The level loads under the logos and the last fade reveals it seamlessly.

While the splash lives, **all input from every device is swallowed**: any key or click skips the current page (jumping to its fade-out, never cutting) if the page allows it, and nothing leaks to the game underneath. Per-page `OnShown` events cover sounds and page-local animations; pages all construct at once, so `Construct` is not the moment a page appears.

### 14. Local Multiplayer

Nothing extra to enable; it is the default model:

- Widgets belong to the player that created them (`CreateWidget` with an owning player, or `SetOwningLocalPlayer`); their input arrives through that player's focus, from that player's devices.
- Each player has an independent widget stack, focus restore policy, input device (and therefore prompt icons), and theme overrides via `UKzUIInputSubsystem::Get(LocalPlayer)`.
- A screen with **`bSoloInput`** suspends every *other* player's UI input while it lives: the player who paused owns the menus; the rest resume automatically when it closes.
- `Add to Player Screen` keeps per-player layouts clean in split-screen; for shared-screen games, `Add to Viewport` from the owning player works equally well.

---

## Notes and Caveats

- **`KzTextBlock` does not support property bindings on `Text`**: call `SetText` instead.
- **The engine console wins**: it takes the viewport focus for itself, so while it is open KzUI never restores focus to the stack, widgets stop consuming input and the stick synthesis stands down. `UKzUIInputSubsystem::IsConsoleActive` exposes the same check.
- **Blueprint-implemented `IKzSelectableWidgetInterface`**: the auto-generated `Is Selectable` stub returns `false`; open it and return your own flag or `true`, or the widget will never be an option.
- **`Switch on Option` enumerates the compiled widget tree**, statically. With a scoped `GetOptions` override, out-of-scope selectables still produce (harmless, never-firing) pins; disable them in the node's details if they bother you.
- **Custom icon token names must not contain `:`**, reserved for the `{Token:Variation}` syntax.
- **Per-platform input overrides resolve by running platform**: PS/Xbox/Switch branches only apply in cooked builds for those platforms.
- **Theme assets referenced from settings**: config paths are not fixed up by redirectors; re-pick the asset in Project Settings if you rename it.
- **Mouse support** is positional (hover under cursor, click to accept) on navigables and prompts; the cursor itself follows the `CursorPolicy`.

---

## Asset Naming

Suggested prefixes, used throughout the docs:

| Prefix | Asset                          |
|--------|--------------------------------|
| `TS_`  | Text style (`KzTextStyle`)     |
| `TSS_` | Text style set (`KzTextBlockStyle`) |
| `IT_`  | Icon theme                     |
| `ST_`  | Sound theme                    |
| `FB_`  | UI flipbook                    |
| `WBP_` | Widget Blueprints              |

---

## Repository Layout

```
KzUI/
├── Source/
│   ├── KzUI/                     # Runtime module
│   │   ├── Public/
│   │   │   ├── KzUITypes.h               # Inputs, devices, states, triggers, key definitions
│   │   │   ├── KzUIInputSettings.h       # Project settings (input map, themes, prompt class)
│   │   │   ├── KzUIInputSubsystem.h      # Per-player stack, focus policy, device tracking
│   │   │   ├── KzUserWidget.h            # Screen base: semantic input, dynamic children, prompts
│   │   │   ├── KzNavigableWidget.h       # Geometric navigation, hover/selection
│   │   │   ├── KzSelectableWidgetInterface.h
│   │   │   ├── KzTextBlock.h             # Styled text with inline icon tokens
│   │   │   ├── KzTextStyle.h             # Style classes and style sets
│   │   │   ├── KzImage.h / KzInputIcon.h / KzUIFlipbook.h
│   │   │   ├── KzUIIconTheme.h / KzUISoundTheme.h
│   │   │   ├── KzOptionWidgets.h         # Settings rows: spinner, slider, toggle
│   │   │   ├── KzSettingsLibrary.h       # GameUserSettings wrappers
│   │   │   ├── KzPromptWidget.h
│   │   │   ├── KzSplashWidget.h / KzSplashPage.h
│   │   │   ├── KzWidgetSwitcher.h / KzSwitcherPage.h
│   │   │   ├── KzInputDeviceListener.h
│   │   │   └── KzUIFunctionLibrary.h
│   │   └── Private/                      # Implementations + FKzUIStickKeyProcessor
│   ├── KzUIUncooked/             # K2 nodes (Switch on Option)
│   └── KzUIEditor/               # Factories and details customizations
├── KzUI.uplugin
├── LICENSE                       # MIT
└── README.md
```

---

## Related Projects

- **[KzLib](https://github.com/kirzo/KzLib)**: the utility library KzUI builds on.
- **[ScriptableFramework](https://github.com/kirzo/ScriptableFramework)**: data-driven gameplay framework from the same family.

---

## Contributing

Contributions are welcome! If you'd like to help:

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/my-feature`.
3. Commit with clear messages and follow the existing code style (Unreal/Epic C++ conventions).
4. Open a Pull Request describing **what** changed and **why**.

For larger changes please open an issue first so we can align on direction before you invest time in the implementation.

---

## License

KzUI is released under the **MIT License**. See [LICENSE](LICENSE) for the full text.

You are free to use it in commercial and non-commercial projects.

---

## Author

Built and maintained by **[Kirzo](https://kirzo.dev/)**.

- 🌐 Website: [kirzo.dev](https://kirzo.dev/)
- 🐙 GitHub: [@kirzo](https://github.com/kirzo)

If KzUI helps your project, consider giving the repository a ⭐. It really helps visibility.