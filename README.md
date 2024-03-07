# fivedxpl
fivedxp linux loader (2.20.02)

- currently only works with xbox controller

# controls
- y = view change
- a = interrupt
- start = service (use this for inserting coin)
- x = card
- left dpad = toggle test menu
- up / down dpad = test up / down
- right dpad = test enter

# tutorial
- copy libfivedxp.so, config.json and start.sh to the game folder
- make a folder named ```libso``` and put libcrypto.so.0.9.8 & libprotobuf.so.7 & libssl.so.0.9.8 in it
- do ```chmod +x start.sh```
- start with ```./start.sh```

# Known problems
- While in attract + online mode, online status is bugging

# TODO
- rewrite jvs + better input handling
- proper terminal fix
- make touch emu
- better input

# credits
https://github.com/jmpews/Dobby
https://github.com/OpenJVS/OpenJVS
https://github.com/drewnoakes/joystick