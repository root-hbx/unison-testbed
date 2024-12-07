#!/bin/bash

tmux new-session -d -s unison-eva-branch-mtp './test-mtp.sh'

tmux new-session -d -s unison-eva-branch-ori './test-ori.sh'

echo "Created tmux sessions: (tmux ls)"
tmux ls

echo "
Some basic tmux commands for record:
- tmux attach -t unison-eva-branch-mtp  # go to mtp session
- tmux attach -t unison-eva-branch-ori  # go to ori session
- Ctrl-b d           # detach from current session, go back to console
- tmux ls           # list all sessions
- tmux kill-session -t unison-eva-branch-mtp  # shut down mtp session
- tmux kill-session -t unison-eva-branch-ori # shut down ori session
- tmux kill-server     # shut down all sessions"
