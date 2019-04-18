# eos-stake-token

SPECIFICATION:
KEY token contract is based on the official 'eosio.token' contract, with some additional functions.
1. stake/unstake
   Staking tokens takes effect immediately. Unstaking tokens takes effect with delay. Unstaking triggers a deferred transaction, the deferred time is configurable.
   In case the deferred transaction does not execute when time is due, anyone can manually execute it by calling 'refund' method.
2. Deferred unstaking requests are stored in table temporarily. A user can have several unstaking requests at the     same time, each request is independent with a unique self-incrementing index as primary key. Unstaking requests    can be cancelled during deferred time.
3. There are 4 types of token transfer, distinguished by memo.
   1) common transfer: only for liquid tokens;
   2) liquid -> staked: transfer FROM's liquid token to TO, automatically become staked, memo format: "Transfer:FromLiquidToStaked";
   3) staked -> staked: transfer FROM's staked token to TO, still staked, memo format: "Transfer:FromStakedToStaked";
   4) staked -> liquid: transfer FROM's staked token to TO, automatically become liquid. In this case transfer fee is required, fee ratio and fee recipient is configurable. memo format: "Transfer:FromStakedToLiquid".


说明文档
key的token合约基于eos官方的token合约开发，增加了抵押等功能。
1. 抵押/解锁代币。
   抵押代币立刻生效。解锁代币需要一段时间，解锁交易会触发一笔延时交易来执行解锁动作。解锁所需要的时间可以配置。
   如果解锁的延时交易没有执行，则任何用户都可以通过refund接口来手动触发。
2. 用户可以同时解锁多笔交易，并且在解锁的过程中，可以取消解锁，多笔解锁交易互相不干扰。
3. 转账代币，有4种模式，通过memo来指定。
   普通转账：只能转账流通的代币。
   流通的代币转账后锁定：只能转账流通的代币，接收的代币会被锁定。memo格式： "Transfer:FromLiquidToStaked"
   锁定的代币转账后锁定：只能转账锁定的代币，接收的代币会被锁定。memo格式： "Transfer:FromStakedToStaked"
   锁定的代币转账后可流通：只能转账锁定的代币，接收的代币可以流通。需要支付一定的手续费。手续费比例和接受账户可以配置。memo格式： "Transfer:FromStakedToLiquid"。 
   
