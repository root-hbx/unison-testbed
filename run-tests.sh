#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

tmux new-session -d -s unison-main-branch-mtp "cd $SCRIPT_DIR && ./test-mtp.sh"

tmux new-session -d -s unison-main-branch-ori "cd $SCRIPT_DIR && ./test-ori.sh"

echo "Created tmux sessions: (tmux ls)"
tmux ls

echo "
Some basic tmux commands for record:
- tmux attach -t unison-main-branch-mtp  # go to mtp session
- tmux attach -t unison-main-branch-ori  # go to ori session
- Ctrl-b d           # detach from current session, go back to console
- tmux ls           # list all sessions
- tmux kill-session -t unison-main-branch-mtp  # shut down mtp session
- tmux kill-session -t unison-main-branch-ori  # shut down ori session
- tmux kill-server     # shut down all sessions"

echo "
Note: There's something important to be aware of here.

The script works smoothly now, but it's actually working by coincidence.

In reality, if commands are run individually:
- MTP will work properly
- ORI will fail, showing error about missing fat-tree-ori object during build

This is because ns-3's build system (waf) requires parent directories to be built first before subdirectories can be used.

Both fat-tree-mtp and fat-tree-ori are located in src/mtp/examples/
However, the mtp module in src/mtp/ needs to be built first.

- When running ./test-mtp.sh, it works because mtp module is built (with --enable-mtp)
- When running ./test-ori.sh alone, it fails because mtp module isn't built (no --enable-mtp)

The reason it works now by coincidence is:

Although we're using two tmux sessions, they're actually running in the same tmux window
(both operating on the same ~/build directory) So when ./test-ori.sh runs, the mtp module is already built,
allowing it to work properly."

echo "
This issue will be correctly fixed in https://github.com/root-hbx/ns-3-dev/blob/unison/run-tests.sh :)"