ARG version

FROM asteroid_rush/base:$version

ARG environment

ADD $environment/boot_config.json /scripts/boot/

ENTRYPOINT cd /scripts/boot && python3 configure.py --data-dir=/opt/eosio/bin/data-dir --wallet-dir=/opt/eosio/bin/data-dir/wallet \
                            --genesis-json=/opt/eosio/bin/data-dir/genesis.json \
                            --keosd="/opt/eosio/bin/keosd --http-server-address=0.0.0.0:8020 --http-alias=localhost:8020" \
                            --nodeos="/opt/eosio/bin/nodeosd.sh \
                                --http-alias=localhost:8000 --http-server-address=0.0.0.0:8000 \
                                --bnet-endpoint=0.0.0.0:8001 --p2p-listen-endpoint=0.0.0.0:8002"  \
                            --cleos="/opt/eosio/bin/cleos --url=http://localhost:8000 --wallet-url=http://localhost:8020" \
                            --contracts-dir="/contracts/"