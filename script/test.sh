cleos wallet unlock -n jungle --password $JunlgePassWD
alias kcleos="cleos -u http://kylin.meet.one:8888"
# alias kcleos="cleos -u https://api-kylin.eosasia.one"
alias curl_balance="curl http://api-kylin.eosasia.one/v1/chain/get_currency_balance -X POST -d"

get_token_balance() {
    curl_balance '{ "code":"hellomykey11", "symbol":"ADD", "account":"hellomykey12"}'
    curl_balance '{ "code":"hellomykey11", "symbol":"ADD", "account":"hellomykey13"}'
    curl_balance '{ "code":"hellomykey11", "symbol":"ADD", "account":"hellomykey14"}'
}

# # create & issue token
# kcleos push action hellomykey11 create '{"issuer":"hellomykey12", "maximum_supply":"1000000.0000 ADD"}'  -p hellomykey11
# kcleos push action hellomykey11 issue '{"to":"hellomykey12", "quantity":"10000.0000 ADD", "memo":""}' -p hellomykey12
# kcleos push action hellomykey11 setdelay '{"symbol":"4,ADD", "t":"10"}' -p hellomykey12
# kcleos push action hellomykey11 settransfee '{"symbol":"4,ADD", "r":"10", "receiver":"hellomykey12"}' -p hellomykey12

# # stake & unstake
# kcleos push action hellomykey11 transfer '{"from":"hellomykey12", "to":"hellomykey13", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey12
# kcleos push action hellomykey11 stake '{"owner":"hellomykey13", "quantity":"40.0000 ADD"}' -p hellomykey13
# kcleos push action hellomykey11 unstake '{"owner":"hellomykey13", "quantity":"30.0000 ADD"}' -p hellomykey13
# kcleos push action hellomykey11 unstake '{"owner":"hellomykey13", "quantity":"10.0000 ADD"}' -p hellomykey13
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey12", "quantity":"61.0000 ADD", "memo":"test"}' -p hellomykey13
# sleep 15
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey12", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey13

# # transfer from liquid to staked
# kcleos push action hellomykey11 transfer '{"from":"hellomykey12", "to":"hellomykey13", "quantity":"100.0000 ADD", "memo":"Transfer:FromLiquidToStaked"}' -p hellomykey12
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey12", "quantity":"1.0000 ADD", "memo":""}' -p hellomykey13
# kcleos push action hellomykey11 unstake '{"owner":"hellomykey13", "quantity":"100.0000 ADD"}' -p hellomykey13
# sleep 15
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey12", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey13


# # transfer from staked to staked
# kcleos push action hellomykey11 transfer '{"from":"hellomykey12", "to":"hellomykey13", "quantity":"100.0000 ADD", "memo":"Transfer:FromLiquidToStaked"}' -p hellomykey12
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey14", "quantity":"100.0000 ADD", "memo":"Transfer:FromStakedToStaked"}' -p hellomykey13
# kcleos push action hellomykey11 unstake '{"owner":"hellomykey14", "quantity":"100.0000 ADD"}' -p hellomykey14
# get_token_balance
# sleep 15
# kcleos push action hellomykey11 transfer '{"from":"hellomykey14", "to":"hellomykey12", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey14
# get_token_balance


# transfer from staked to liquid
kcleos push action hellomykey11 transfer '{"from":"hellomykey12", "to":"hellomykey13", "quantity":"110.0000 ADD", "memo":"Transfer:FromLiquidToStaked"}' -p hellomykey12
kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey14", "quantity":"100.0000 ADD", "memo":"Transfer:FromStakedToLiquid"}' -p hellomykey13
get_token_balance
kcleos push action hellomykey11 transfer '{"from":"hellomykey14", "to":"hellomykey12", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey14
get_token_balance






echo "done"
cleos wallet lock -n jungle 



# kcleos push action hellomykey11 unstake '{"owner":"hellomykey13", "quantity":"100.0000 ADD"}' -p hellomykey13
# sleep 15
# kcleos push action hellomykey11 transfer '{"from":"hellomykey13", "to":"hellomykey12", "quantity":"100.0000 ADD", "memo":"test"}' -p hellomykey13
# get_token_balance