/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include <string>
using namespace eosio;
using std::string;

class [[eosio::contract]] token : public contract {
   public:
      using contract::contract;

      [[eosio::action]]
      void create( name   issuer,
                     asset  maximum_supply);

      [[eosio::action]]
      void issue( name to, asset quantity, string memo );

      [[eosio::action]]
      void retire( asset quantity, string memo );


      [[eosio::action]]
      void setdelay(const symbol& symbol, uint64_t t);

      [[eosio::action]]
      void settransfee(const symbol& symbol, uint64_t r, name receiver);

      [[eosio::action]]
      void transfer( name    from,
                     name    to,
                     asset   quantity,
                     string  memo );

      [[eosio::action]]
      void stake(name owner, asset quantity);

      [[eosio::action]]
      void autostake(name owner, asset quantity);

      [[eosio::action]]
      void unstake(name owner, asset quantity);

      [[eosio::action]]
      void refund(name caller, name owner, uint64_t index); 

      [[eosio::action]]
      void autorefund(name owner, uint64_t index); 

      [[eosio::action]]
      void cancelunstake(name owner, uint64_t index);

      [[eosio::action]]
      void open( name owner, const symbol& symbol, name ram_payer );

      [[eosio::action]]
      void close( name owner, const symbol& symbol );

      static asset get_supply( name token_contract_account, symbol_code sym_code )
      {
         stats statstable( token_contract_account, sym_code.raw() );
         const auto& st = statstable.get( sym_code.raw() );
         return st.supply;
      }

      static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw() );
         return ac.balance;
      }

   private:
      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] lock_account {
         asset    locked_balance;

         uint64_t primary_key()const { return locked_balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;
         uint64_t refund_delay;
         uint64_t transfer_fee_ratio;
         name fee_receiver;

         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };
      
      struct [[eosio::table]] refund_request {
         uint64_t index;
         name  owner;
         time_point_sec  available_time;
         eosio::asset  amount;

         uint64_t  primary_key()const { return index; }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( refund_request, (index)(owner)(available_time)(amount) )
      };

      typedef eosio::multi_index< name("accounts"), account > accounts;
      typedef eosio::multi_index< name("lockaccounts"), lock_account > lock_accounts;
      typedef eosio::multi_index< name("stat"), currency_stats > stats;
      typedef eosio::multi_index< name("refunds"), refund_request >  refunds_table;

      void sub_balance( name owner, asset value , bool use_locked_balance = false);
      void add_balance( name owner, asset value, name ram_payer );

      void inline_refund(name owner, name rampayer, uint64_t index);
      void inline_stake(name owner, asset quantity, name rampayer);

      void transfer_liquid_to_staked(name from, name to, asset quantity);
      void transfer_staked_to_staked(name from, name to, asset quantity);
      void transfer_staked_to_liquid(name from, name to, asset quantity);
      asset collect_refund(name owner, const symbol& symbol, uint64_t auto_index);
};

