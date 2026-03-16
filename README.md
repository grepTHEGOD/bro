# bro

bro is a version control system for people who think git is boring. No offense to Linus Torvalds.

bro gives you all the power of git, but with commands that actually make sense. Instead of `git commit`, you `yeet`. Instead of `git push`, you `launch`. Instead of `git pull`, you `absorb`. You get the vibe.

## Building

```bash
# Using CMake (recommended)
mkdir build && cd build
cmake ..
make

# Or just use the existing Makefile
make
```

### Installing with CMake

```bash
# System-wide installation
sudo cmake --install . --prefix /usr

# Or using make install
cd build
sudo make install
```

This will produce the `bro` executable.

## Quick Start

```bash
# Initialize a new repo
bro init

# Make some changes to your files...
# Then stage them
bro slap <file>

# Commit your changes
bro yeet -m "your message"

# See what's going on
bro vibe-check

# Push to remote
bro launch

# Get latest from remote
bro absorb
```

## Commands

Here's the full command reference. Every bro command maps to a git command:

### Basics

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro init` | `git init` | Start a new repo vibe |
| `bro slap` | `git add` | Stage your changes |
| `bro yeet` | `git commit` | Save your changes forever |
| `bro vibe-check` | `git status` | See what's going on |
| `bro yap` | `git log` | Read the history |

### Branching & Remotes

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro squad` | `git branch` | List your branches |
| `bro new-squad <name>` | `git branch <name>` | Create a new branch |
| `bro teleport` | `git checkout / git switch` | Jump to a different branch/commit |
| `bro launch` | `git push` | Send code to the cloud |
| `bro absorb` | `git pull` | Get the latest updates |
| `bro yoink` | `git fetch` | Download info, don't merge yet |
| `bro steal` | `git clone` | Copy someone else's work |
| `bro force-launch` | `git push --force` | Force the cloud to listen |

### Merging & History

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro fuse` | `git merge` | Combine two branches |
| `bro rewind` | `git rebase` | Re-write history nicely |
| `bro yoink-cherry` | `git cherry-pick` | Steal one specific commit |

### Undo Operations

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro undo-vibes` | `git reset` | Back up a bit |
| `bro soft-undo` | `git reset --soft` | Undo commit, keep changes staged |
| `bro hard-undo` | `git reset --hard` | Wipe everything to last commit |
| `bro oopsie` | `git revert` | Undo a specific commit |
| `bro panic-revert` | `git revert HEAD` | Emergency undo |
| `bro amend-yeet` | `git commit --amend` | Fix the last commit |

### File Operations

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro delete-bro` | `git rm` | Delete a file |
| `bro yeet-file` | `git mv` | Move/Rename a file |
| `bro untrack-bro` | `git rm --cached` | Stop tracking but keep the file |
| `bro scrub` | `git clean` | Wipe untracked files |
| `bro hide` | `git stash` | Save for later, hide from view |

### Inspection

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro roast` | `git diff` | See what you actually changed |
| `bro diff-bro` | `git diff --cached` | See what's ready to yeet |
| `bro peek` | `git cat-file` | Look inside an object |
| `bro expose` | `git show` | Inspect a specific object |
| `bro hash-slap` | `git hash-object` | Generate a SHA-1 hash |
| `bro ls-bro` | `git ls-files` | List all tracked files |

### Tags & Versions

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro drip` | `git tag` | Mark a version |
| `bro nickname` | `git describe` | Give a human name to a commit |

### Debugging

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro detective` | `git bisect` | Find where it all went wrong |
| `bro snitch` | `git blame` | See who wrote that line |
| `bro search-bro` | `git grep` | Find text in the code |
| `bro grep-it` | `git grep` | Specialized search |
| `bro blame-grep` | `git blame + grep` | Advanced investigation |
| `bro bro-doctor` | `git fsck` | Check for corruption |

### Advanced

| bro Command | Git Equivalent | Description |
|-------------|-----------------|--------------|
| `bro summary` | `git shortlog` | Summarize the yap |
| `bro who-dis` | `git shortlog -sn` | See who's contributing the most |
| `bro reflog-yap` | `git reflog` | The real history (even deletes) |
| `bro time-machine` | `git reflog + reset` | Go back to a specific state |
| `bro patch-mail` | `git format-patch` | Prepare patches for email |
| `bro apply-mail` | `git am` | Apply patches from email |
| `bro sticky-note` | `git notes` | Add notes to commits |
| `bro sidekick` | `git submodule` | Repo inside a repo |
| `bro multiverse` | `git worktree` | Multiple branches at once |
| `bro only-fans` | `git sparse-checkout` | Only download what you need |
| `bro zip-pack` | `git bundle` | Repo in a single file |
| `bro range-roast` | `git range-diff` | Compare two versions of a patch |
| `bro archive-vibes` | `git archive` | Export the repo to a zip |
| `bro clean-house` | `git gc` | Garbage collection / optimize |
| `bro ignore-bro` | `.gitignore` | Management of ignored files |
| `bro config-vibe` | `git config` | Change your settings |

### Special Modes

#### Interactive TUI

```bash
bro tui
```

Launch an interactive terminal UI (similar to tig or lazygit) for a visual git experience with:
- Branch graph visualization
- Multiple views: Commits, Branches, Stashes, Remotes, Stats
- Sidebar with quick actions
- Detail panel for selected items
- Keyboard navigation (j/k for up/down, 1-5 for views)

#### Live Server (IDE Integration)

```bash
bro live
```

Start a background server mode for IDE integration (similar to a Git Language Server).

#### broIDE - Vim-Based IDE

```bash
bro vim-ide
```

Launch a full-featured integrated development environment with:
- **File Explorer**: Browse and navigate project files
- **Tab Management**: Open multiple files in tabs
- **Vim-style Navigation**: h/j/k/l for cursor movement, i for insert mode
- **Built-in Terminal**: Run commands without leaving the editor
- **Status Bar**: Show cursor position, file status, mode
- **Syntax Highlighting**: Color-coded code display

Controls:
- `F1` - Toggle terminal
- `F2` - Files view
- `F3` - Search
- `Tab` - Switch tabs
- `Enter` - Open file
- `:w` - Save file
- `q` - Quit

#### broManager - Milanote-Style Project Editor

```bash
bro milanote
```

Open a visual Kanban-style project board editor with:
- **Multiple Boards**: TODO, IN PROGRESS, DONE columns
- **Card Management**: Create, edit, delete, prioritize cards
- **Task Tracking**: Mark tasks complete, track progress
- **Visual Layout**: Cards displayed in columns with descriptions
- **Progress Stats**: See completion percentage at a glance

Controls:
- `j/k` or `↑/↓` - Navigate cards
- `h/l` or `←/→` - Switch boards
- `Tab` - Next board
- `1/2/3` - Jump to specific board
- `n` - New card
- `e` - Edit/cycle priority
- `d` - Delete card
- `Space` - Toggle done

#### broMail - Thunderbird Email Integration

```bash
bro thunderbird
```

Access email functionality for patch management and collaboration with:
- **Folder Navigation**: Inbox, Sent, Drafts, Trash, Archive, Junk
- **Email List**: View emails with sender, subject, timestamp
- **Email Preview**: Read full email content
- **Compose**: Write and send new emails
- **Actions**: Reply, Forward, Flag, Star, Delete
- **Unread Tracking**: Show unread counts per folder

Controls:
- `j/k` or `↑/↓` - Navigate emails
- `←/→` or `g/$` - Switch folders
- `c` - Compose new email
- `r` - Reply
- `d` - Delete
- `f` - Flag
- `s` - Star
- `u` - Toggle read/unread
- `q` - Quit

## Configuration

Configure bro settings:

```bash
bro config-vibe user.name "Your Name"
bro config-vibe user.email "you@example.com"
```

## Repository Structure

When you run `bro init`, it creates a `.bro` directory with:

```
.bro/
├── HEAD           # Current branch reference
├── config         # Repository configuration
├── info/          # Additional info
│   └── exclude    # Patterns to ignore locally
├── objects/       # All your data (blobs, trees, commits)
└── refs/          # Pointers to commits
    ├── heads/     # Local branches
    └── tags/      # Tags
```

## Why bro?

Because versioning your code should be fun, not a chore. 

> "bro, did you commit that?"
> 
> "yeah bro, I yeeted it"

## License

MIT. Go forth and yeet responsibly.
