# fivedxpl
fivedxp linux loader (5dx+ 2.20.02 & 4 1.10.08)

- currently only works with controllers
- use a keyboard to controller software if you want to use keyboard

# xbox controls
- y = view change
- a = interrupt
- x = service (use this for inserting coin)
- b = card
- left shoulder = shift gear down
- right shoulder = shift gear up
- left dpad = toggle test menu
- up / down dpad = test up / down
- right dpad = test enter

# setup
- copy libfivedxp.so, config.json and start.sh to the game folder
- make a folder named ```libso``` and put libcrypto.so.0.9.8 & libprotobuf.so.7 & libssl.so.0.9.8 in it
- do ```chmod +x start.sh```
- start the game
- enable test menu
- open game options
- disable ic card r/w use (if offline mode)
- disable ic card vendor use
- disable steering power
- open i/o test and go to i/o interface initialize

# starting the game
- ```./start.sh```

# TODO
- fix sound getting muted sometimes
- rewrite jvs
- proper terminal fix for 5dxp

# credits
https://github.com/jmpews/Dobby
https://github.com/OpenJVS/OpenJVS