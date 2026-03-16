// src/main.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Command declarations
int cmd_init(int argc, char **argv);
int cmd_slap(int argc, char **argv);
int cmd_yeet(int argc, char **argv);
int cmd_vibe_check(int argc, char **argv);
int cmd_yap(int argc, char **argv);
int cmd_squad(int argc, char **argv);
int cmd_new_squad(int argc, char **argv);
int cmd_teleport(int argc, char **argv);
int cmd_launch(int argc, char **argv);
int cmd_absorb(int argc, char **argv);
int cmd_yoink(int argc, char **argv);
int cmd_steal(int argc, char **argv);
int cmd_force_launch(int argc, char **argv);
int cmd_fuse(int argc, char **argv);
int cmd_rewind(int argc, char **argv);
int cmd_yoink_cherry(int argc, char **argv);
int cmd_undo_vibes(int argc, char **argv);
int cmd_soft_undo(int argc, char **argv);
int cmd_hard_undo(int argc, char **argv);
int cmd_oopsie(int argc, char **argv);
int cmd_panic_revert(int argc, char **argv);
int cmd_amend_yeet(int argc, char **argv);
int cmd_delete_bro(int argc, char **argv);
int cmd_yeet_file(int argc, char **argv);
int cmd_untrack_bro(int argc, char **argv);
int cmd_scrub(int argc, char **argv);
int cmd_hide(int argc, char **argv);
int cmd_roast(int argc, char **argv);
int cmd_diff_bro(int argc, char **argv);
int cmd_peek(int argc, char **argv);
int cmd_expose(int argc, char **argv);
int cmd_hash_slap(int argc, char **argv);
int cmd_ls_bro(int argc, char **argv);
int cmd_drip(int argc, char **argv);
int cmd_nickname(int argc, char **argv);
int cmd_detective(int argc, char **argv);
int cmd_snitch(int argc, char **argv);
int cmd_search_bro(int argc, char **argv);
int cmd_grep_it(int argc, char **argv);
int cmd_blame_grep(int argc, char **argv);
int cmd_bro_doctor(int argc, char **argv);
int cmd_summary(int argc, char **argv);
int cmd_who_dis(int argc, char **argv);
int cmd_reflog_yap(int argc, char **argv);
int cmd_time_machine(int argc, char **argv);
int cmd_patch_mail(int argc, char **argv);
int cmd_apply_mail(int argc, char **argv);
int cmd_sticky_note(int argc, char **argv);
int cmd_sidekick(int argc, char **argv);
int cmd_multiverse(int argc, char **argv);
int cmd_zip_pack(int argc, char **argv);
int cmd_range_roast(int argc, char **argv);
int cmd_archive_vibes(int argc, char **argv);
int cmd_clean_house(int argc, char **argv);
int cmd_ignore_bro(int argc, char **argv);
int cmd_config_vibe(int argc, char **argv);
int cmd_tui(int argc, char **argv);
int cmd_live(int argc, char **argv);

int cmd_vim_ide(int argc, char **argv);
int cmd_milanote_editor(int argc, char **argv);
int cmd_thunderbird_email(int argc, char **argv);

// Forward declarations for new features
void start_vim_ide();
void start_milanote_editor();
void start_thunderbird_email();

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("bro: missing command lil bro\nKnown moves: init, slap, yeet, vibe-check, yap, squad, new-squad, teleport, launch, absorb, yoink, steal, force-launch, fuse, rewind, yoink-cherry, undo-vibes, soft-undo, hard-undo, oopsie, panic-revert, amend-yeet, delete-bro, yeet-file, untrack-bro, scrub, hide, roast, diff-bro, peek, expose, hash-slap, ls-bro, drip, nickname, detective, snitch, search-bro, grep-it, blame-grep, bro-doctor, summary, who-dis, reflog-yap, time-machine, patch-mail, apply-mail, sticky-note, sidekick, multiverse, zip-pack, range-roast, archive-vibes, clean-house, ignore-bro, config-vibe, tui, live, vim-ide, milanote, thunderbird");
        return 1;
    }

    const char *cmd = argv[1];

    // Basics
    if (strcmp(cmd, "init") == 0) return cmd_init(argc - 1, argv + 1);
    if (strcmp(cmd, "slap") == 0) return cmd_slap(argc - 1, argv + 1);
    if (strcmp(cmd, "yeet") == 0) return cmd_yeet(argc - 1, argv + 1);
    if (strcmp(cmd, "vibe-check") == 0) return cmd_vibe_check(argc - 1, argv + 1);
    if (strcmp(cmd, "yap") == 0) return cmd_yap(argc - 1, argv + 1);
    
    // Branching & Remotes
    if (strcmp(cmd, "squad") == 0) return cmd_squad(argc - 1, argv + 1);
    if (strcmp(cmd, "new-squad") == 0) return cmd_new_squad(argc - 1, argv + 1);
    if (strcmp(cmd, "teleport") == 0) return cmd_teleport(argc - 1, argv + 1);
    if (strcmp(cmd, "launch") == 0) return cmd_launch(argc - 1, argv + 1);
    if (strcmp(cmd, "absorb") == 0) return cmd_absorb(argc - 1, argv + 1);
    if (strcmp(cmd, "yoink") == 0) return cmd_yoink(argc - 1, argv + 1);
    if (strcmp(cmd, "steal") == 0) return cmd_steal(argc - 1, argv + 1);
    if (strcmp(cmd, "force-launch") == 0) return cmd_force_launch(argc - 1, argv + 1);
    
    // Merging & History
    if (strcmp(cmd, "fuse") == 0) return cmd_fuse(argc - 1, argv + 1);
    if (strcmp(cmd, "rewind") == 0) return cmd_rewind(argc - 1, argv + 1);
    if (strcmp(cmd, "yoink-cherry") == 0) return cmd_yoink_cherry(argc - 1, argv + 1);
    
    // Undo Operations
    if (strcmp(cmd, "undo-vibes") == 0) return cmd_undo_vibes(argc - 1, argv + 1);
    if (strcmp(cmd, "soft-undo") == 0) return cmd_soft_undo(argc - 1, argv + 1);
    if (strcmp(cmd, "hard-undo") == 0) return cmd_hard_undo(argc - 1, argv + 1);
    if (strcmp(cmd, "oopsie") == 0) return cmd_oopsie(argc - 1, argv + 1);
    if (strcmp(cmd, "panic-revert") == 0) return cmd_panic_revert(argc - 1, argv + 1);
    if (strcmp(cmd, "amend-yeet") == 0) return cmd_amend_yeet(argc - 1, argv + 1);
    
    // File Operations
    if (strcmp(cmd, "delete-bro") == 0) return cmd_delete_bro(argc - 1, argv + 1);
    if (strcmp(cmd, "yeet-file") == 0) return cmd_yeet_file(argc - 1, argv + 1);
    if (strcmp(cmd, "untrack-bro") == 0) return cmd_untrack_bro(argc - 1, argv + 1);
    if (strcmp(cmd, "scrub") == 0) return cmd_scrub(argc - 1, argv + 1);
    if (strcmp(cmd, "hide") == 0) return cmd_hide(argc - 1, argv + 1);
    
    // Inspection
    if (strcmp(cmd, "roast") == 0) return cmd_roast(argc - 1, argv + 1);
    if (strcmp(cmd, "diff-bro") == 0) return cmd_diff_bro(argc - 1, argv + 1);
    if (strcmp(cmd, "peek") == 0) return cmd_peek(argc - 1, argv + 1);
    if (strcmp(cmd, "expose") == 0) return cmd_expose(argc - 1, argv + 1);
    if (strcmp(cmd, "hash-slap") == 0) return cmd_hash_slap(argc - 1, argv + 1);
    if (strcmp(cmd, "ls-bro") == 0) return cmd_ls_bro(argc - 1, argv + 1);
    
    // Tags & Versions
    if (strcmp(cmd, "drip") == 0) return cmd_drip(argc - 1, argv + 1);
    if (strcmp(cmd, "nickname") == 0) return cmd_nickname(argc - 1, argv + 1);
    
    // Debugging
    if (strcmp(cmd, "detective") == 0) return cmd_detective(argc - 1, argv + 1);
    if (strcmp(cmd, "snitch") == 0) return cmd_snitch(argc - 1, argv + 1);
    if (strcmp(cmd, "search-bro") == 0) return cmd_search_bro(argc - 1, argv + 1);
    if (strcmp(cmd, "grep-it") == 0) return cmd_grep_it(argc - 1, argv + 1);
    if (strcmp(cmd, "blame-grep") == 0) return cmd_blame_grep(argc - 1, argv + 1);
    if (strcmp(cmd, "bro-doctor") == 0) return cmd_bro_doctor(argc - 1, argv + 1);
    
    // Advanced
    if (strcmp(cmd, "summary") == 0) return cmd_summary(argc - 1, argv + 1);
    if (strcmp(cmd, "who-dis") == 0) return cmd_who_dis(argc - 1, argv + 1);
    if (strcmp(cmd, "reflog-yap") == 0) return cmd_reflog_yap(argc - 1, argv + 1);
    if (strcmp(cmd, "time-machine") == 0) return cmd_time_machine(argc - 1, argv + 1);
    if (strcmp(cmd, "patch-mail") == 0) return cmd_patch_mail(argc - 1, argv + 1);
    if (strcmp(cmd, "apply-mail") == 0) return cmd_apply_mail(argc - 1, argv + 1);
    if (strcmp(cmd, "sticky-note") == 0) return cmd_sticky_note(argc - 1, argv + 1);
    if (strcmp(cmd, "sidekick") == 0) return cmd_sidekick(argc - 1, argv + 1);
    if (strcmp(cmd, "multiverse") == 0) return cmd_multiverse(argc - 1, argv + 1);
    if (strcmp(cmd, "zip-pack") == 0) return cmd_zip_pack(argc - 1, argv + 1);
    if (strcmp(cmd, "range-roast") == 0) return cmd_range_roast(argc - 1, argv + 1);
    if (strcmp(cmd, "archive-vibes") == 0) return cmd_archive_vibes(argc - 1, argv + 1);
    if (strcmp(cmd, "clean-house") == 0) return cmd_clean_house(argc - 1, argv + 1);
    if (strcmp(cmd, "ignore-bro") == 0) return cmd_ignore_bro(argc - 1, argv + 1);
    if (strcmp(cmd, "config-vibe") == 0) return cmd_config_vibe(argc - 1, argv + 1);
    
    // Special Modes
    if (strcmp(cmd, "tui") == 0) return cmd_tui(argc - 1, argv + 1);
    if (strcmp(cmd, "live") == 0) return cmd_live(argc - 1, argv + 1);

    // New Features
    if (strcmp(cmd, "vim-ide") == 0) return cmd_vim_ide(argc - 1, argv + 1);
    if (strcmp(cmd, "milanote") == 0) return cmd_milanote_editor(argc - 1, argv + 1);
    if (strcmp(cmd, "thunderbird") == 0) return cmd_thunderbird_email(argc - 1, argv + 1);

    fprintf(stderr, "bro: '%s' not recognized... yet 😈\n", cmd);
    return 1;
}


