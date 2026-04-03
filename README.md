# Swindings

**View sway keybindings**

![demo](public/site/demo.mp4)

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
