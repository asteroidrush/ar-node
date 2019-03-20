
# Asteroid Rush - EOSIO-based blockchain

## EOSIO version

We are based on eosio v1.5.2.  


## Changes

### Blockchain core

We have no any breaking changes in core, except modification allowing to restrict accounts who may upload contracts.

### System contracts

* Another core-token named PSTR (PiAstro).
* We are using DPOS, but have changed algorithm of staking. We excluded resource staking and you may vote using your core-tokens.
* Right to create account has only eosio account.
* Every user account has 3Kb RAM and equal part of NET and CPU with other users.
* All RAM costs paid by contract.
* RAM, NET, CPU may be set directly by eosio account, which is controlled by blockchain government.

There is no public testnet running currently.

## Build

All instructions about build process same with EOSIO project, so you can refer [original docs](https://developers.eos.io/eosio-nodeos/docs/autobuild-script).

If you prefer docker you can look at our github [docker repository](https://github.com/asteroidrush/ar-node-docker).


## Supported Operating Systems
EOSIO currently supports the following operating systems:  
1. Amazon 2017.09 and higher  
2. Centos 7  
3. Fedora 25 and higher (Fedora 27 recommended)  
4. Mint 18  
5. Ubuntu 16.04 (Ubuntu 16.10 recommended)  
6. Ubuntu 18.04  
7. MacOS Darwin 10.12 and higher (MacOS 10.13.x recommended)  

## Resources
1. [Website](https://asteroidrush.io)
1. [Blog](https://medium.com/@asteroidrushgame)
1. [Community Discord Group](https://discord.gg/D2UJ8Wx)
1. [White Paper](https://asteroidrush.io/whitepaper)
1. [Roadmap](https://asteroidrush.io/#roadmap)

