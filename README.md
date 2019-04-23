# eos-stake-token


## Specification

Stake token contract is based on the official 'eosio.token' contract, with some additional functions to realize basic token economy mechanism.

### 1. stake / unstake

   Staking tokens takes effect immediately. Unstaking tokens takes effect with delay. Unstaking triggers a deferred transaction, the deferred time is configurable.
   
   In case the deferred transaction does not execute when time is due, anyone can manually execute it by calling 'refund' method.
   
### 2. Deferred unstaking requests are stored in table temporarily. 
A user can have several unstaking requests at the same time, each request is independent with a unique self-incrementing index as primary key. Unstaking requests can be cancelled during deferred period.

### 3. There are 3 types of token transfer, distinguished by memo.

   - **Common transfer**: Only for liquid tokens;
   - **Liquid -> Staked**: Transfer `FROM`'s liquid token to `TO`, automatically become staked. If `TO` is in stake blacklist, this transfer will fail, token issuer can call 'addblacklist'/'rmblacklist' to manage blacklist. Transfer memo format: `"Transfer:FromLiquidToStaked"`;
   - **Staked -> Liquid**: Transfer `FROM`'s staked token to `TO`, automatically become liquid. In this case transfer fee is required, fee ratio and fee recipient is configurable. Transfer memo format: `"Transfer:FromStakedToLiquid"`.


## Extra features in MYKEY

If Smart Contract of dapps use the tranfer protocol in this sample, they will get build-in features and better experiences in MYKEY App. 

Their dapps can use upcoming MYKEY SDK to build safe and solid dapps with basic token economy mechanism rapidly.

## Who are using?

**KEY token**

more...

   
## Thanks!
 
Special thanks to the community contributors help to audit and verify it.
 
**EOS Authority**
 
**HKEOS** 
 
**EOS 42**
 
**EOS Sweden**
 
**EOS Cannon**


 
 

