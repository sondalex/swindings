# Swindings

**View sway keybindings**

[demo](https://github.com/user-attachments/assets/71a746ae-131a-4b7b-85b7-2330b63ebcd1)

## Installation

### Prerequisites

- Sway window manager (Wayland)

### One line install (recommended)

```
curl -sSfL https://sondalex.github.io/swindings/install.sh | sh
```


## Usage

Run:

```
swindings
```


## Theming

Those themes are available:

**Default**
![Default](assets/default.png)
---

**Tokyo-Night**
![Tokyo-Night](assets/tokyo-night.png)
---
**Solarized Dark**
![Solarized Dark](assets/solarized-dark.png)
---
**Catppuccin Mocha**
![Catppuccin Mocha](assets/catppuccin-mocha.png)
---
**[Catppuccin Latte**
![Catppuccin Latte](assets/catppuccin-latte.png)
---
**Dracula**
![Dracula](assets/dracula.png)
---
**Gruvbox Dark**
![Gruvbox Dark](assets/gruvbox-dark.png)
---
**Nord**
![Nord](assets/nord.png)
---


If you want to switch for Tokyo-Night:

```bash
cp config/tokyo-night.toml ~/.config/swindings/config.toml
```


## Recommendation

Add keymap to your `~/.config/sway/config`:

```ini
bindsym $mod+k+m swindings
```

## Building from source

```bash
git clone https://github.com/sondalex/swindings.git
git submodule update --init --recursive
```

## Development

### Testing

```bash
zig build test
```

### Generating compile_commands.json

```bash
zig build cdb
```
