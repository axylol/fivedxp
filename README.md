# fivedxpl
fivedxp linux loader (5dx+ 2.20.02 & 4 1.10.08)
https://nightly.link/feathercx/fivedxp/workflows/build/master/release.zip

# Common Sense is REQUIRED
### if your still using config.json, you will need to change it to config.toml

# setup
- copy everything in Dist folder and libfivedxp.so to the game folder
- make a folder named ```libso``` and put libcrypto.so.0.9.8 & libprotobuf.so.7 & libssl.so.0.9.8 in it
- do ```chmod +x start.sh```
- start the game
- enable test menu
- open game options
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