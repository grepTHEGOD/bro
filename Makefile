CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc
LDFLAGS = -lz -ljson-c -lncurses -lcurl

SRC = src/main.c \
      src/util.c \
      src/github.c \
      src/repo.c \
      src/refs.c \
      src/index.c \
      src/remote.c \
      src/core/blob.c \
      src/core/commit.c \
      src/core/object.c \
      src/core/odb.c \
      src/core/sha1.c \
      src/core/tree.c \
      src/core/zlib_utils.c \
      src/commands/init.c \
      src/commands/slap.c \
      src/commands/yeet.c \
      src/commands/vibe-check.c \
      src/commands/yap.c \
      src/commands/squad.c \
      src/commands/new-squad.c \
      src/commands/teleport.c \
      src/commands/launch.c \
      src/commands/absorb.c \
      src/commands/yoink.c \
      src/commands/steal.c \
      src/commands/force-launch.c \
      src/commands/fuse.c \
      src/commands/rewind.c \
      src/commands/yoink-cherry.c \
      src/commands/undo-vibes.c \
      src/commands/soft-undo.c \
      src/commands/hard-undo.c \
      src/commands/oopsie.c \
      src/commands/panic-revert.c \
      src/commands/amend-yeet.c \
      src/commands/delete-bro.c \
      src/commands/yeet-file.c \
      src/commands/untrack-bro.c \
      src/commands/scrub.c \
      src/commands/hide.c \
      src/commands/roast.c \
      src/commands/diff-bro.c \
      src/commands/peek.c \
      src/commands/expose.c \
      src/commands/hash-slap.c \
      src/commands/ls-bro.c \
      src/commands/drip.c \
      src/commands/nickname.c \
      src/commands/detective.c \
      src/commands/snitch.c \
      src/commands/search-bro.c \
      src/commands/grep-it.c \
      src/commands/blame-grep.c \
      src/commands/bro-doctor.c \
      src/commands/summary.c \
      src/commands/who-dis.c \
      src/commands/reflog-yap.c \
      src/commands/time-machine.c \
      src/commands/patch-mail.c \
      src/commands/apply-mail.c \
      src/commands/sticky-note.c \
      src/commands/sidekick.c \
      src/commands/multiverse.c \
      src/commands/zip-pack.c \
      src/commands/range-roast.c \
      src/commands/archive-vibes.c \
      src/commands/clean-house.c \
      src/commands/ignore-bro.c \
      src/commands/config-vibe.c \
      src/commands/tui.c \
      src/commands/live.c

OBJ = $(SRC:.c=.o)

all: bro

bro: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) bro

.PHONY: all clean
