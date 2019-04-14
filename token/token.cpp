/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "token.hpp"

#define SENDER_ID(X, Y)        ( ((uint128_t)X << 64) | Y )

time_point current_time_point() {
   const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
   return ct;
}


void token::create( name   issuer,
                    asset  maximum_supply )
{
      require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
       s.refund_delay  = 0;
       s.transfer_fee_ratio  = 0;
       s.fee_receiver      = issuer;
    });
}


void token::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void token::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

/* 
 * 4 transfer modes in stake-token
 * 1) transfer from liquid to liquid as usual,
 * 2) transfer from liquid to staked, received token is staked
 * 3) transfer from staked to staked, send staked token and received token is staked 
 * 4) transfer from staked to liquid, send staked token and received token is liquid, fee is required.
 */
void token::transfer( name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
   eosio_assert( from != to, "cannot transfer to self" );
   require_auth( from );
   eosio_assert( is_account( to ), "to account does not exist");
   auto sym = quantity.symbol.code();
   stats statstable( _self, sym.raw() );
   const auto& st = statstable.get( sym.raw() );

   require_recipient( from );
   require_recipient( to );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

   auto payer = has_auth( to ) ? to : from;

   auto separator_pos = memo.find(':');

   if(separator_pos != string::npos) {
      string main_memo = memo.substr(0, separator_pos);
      string sub_memo = memo.substr(separator_pos + 1);

      if (main_memo == "Transfer" && sub_memo == "FromLiquidToStaked") {
         return transfer_liquid_to_staked(from, to, quantity);
      } else if (main_memo == "Transfer" && sub_memo == "FromStakedToStaked") {
         return transfer_staked_to_staked(from, to, quantity);
      } else if (main_memo == "Transfer" && sub_memo == "FromStakedToLiquid") {
         return transfer_staked_to_liquid(from, to, quantity);
      }
   }

   // default transfer
   sub_balance( from, quantity );
   add_balance( to, quantity, payer );

}

void token::transfer_liquid_to_staked(name from, name to, asset quantity)
{
   sub_balance( from, quantity);
   add_balance( to, quantity, from );

   action lock_action = action(
      permission_level{ _self, name("active")},
      _self,
      name("autostake"),
      std::make_tuple(to, quantity)
   );
   lock_action.send();
}

void token::transfer_staked_to_liquid(name from, name to, asset quantity)
{
   //locked
   lock_accounts lock_from_acnts( _self, from.value );
   const auto& lock_from = lock_from_acnts.get( quantity.symbol.code().raw(), "no balance object found in lock_accounts" );

   //ongoing unstaking
   refunds_table refunds_tbl( _self, from.value );
   uint64_t auto_index = refunds_tbl.available_primary_key();
   asset unstaking_amount = collect_refund(from, quantity.symbol, auto_index);

   //transfer fee
   const auto& sym_code_raw = quantity.symbol.code().raw();
   stats statstable( _self, sym_code_raw);
   const auto& st = statstable.get( sym_code_raw , "symbol does not exist");
   const auto& transfer_fee = quantity * st.transfer_fee_ratio / 100;
   eosio_assert( lock_from.locked_balance >= ( quantity + unstaking_amount + transfer_fee), "transfer_staked_to_liquid overdrawn balance" );

   sub_balance( from, quantity, true);
   add_balance( to, quantity, from );

   //subtract from's locked balance
   lock_from_acnts.modify( lock_from, from, [&]( auto& a ) {
      a.locked_balance -= (quantity + transfer_fee);
   });

   if(transfer_fee.amount > 0) {
      action act = action(
         permission_level{ from, name("active")},
         _self,
         name("transfer"),
         std::make_tuple(from, st.fee_receiver, transfer_fee, std::string("Transfer:FromStakedToLiquid fee"))
      );
      act.send();
   }

   return;
}

void token::transfer_staked_to_staked(name from, name to, asset quantity)
{
   sub_balance( from, quantity, true);
   add_balance( to, quantity, from );

   //locked
   lock_accounts lock_from_acnts( _self, from.value );
   const auto& lock_from = lock_from_acnts.get( quantity.symbol.code().raw(), "no balance object found in lock_accounts" );

   //ongoing unstaking
   refunds_table refunds_tbl( _self, from.value );
   uint64_t auto_index = refunds_tbl.available_primary_key();
   eosio_assert( auto_index >= 0, "auto_index invalid " );

   asset unstaking_amount = collect_refund(from, quantity.symbol, auto_index);
   eosio_assert( lock_from.locked_balance >= ( quantity + unstaking_amount), "transfer_staked_to_staked overdrawn balance" );

   //subtract from's locked balance
   lock_from_acnts.modify( lock_from, from, [&]( auto& a ) {
      a.locked_balance -= quantity;
   });

   //add to's locked balance
   action lock_action = action(
      permission_level{ _self, name("active")},
      _self,
      name("autostake"),
      std::make_tuple(to, quantity)
   );
   lock_action.send();
}

void token::inline_stake(name owner, asset quantity, name rampayer)
{
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must stake positive quantity" );

   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( quantity.symbol.code().raw(), "no balance object found" );

   lock_accounts lock_from_acnts( _self, owner.value );
   const auto& lock_from = lock_from_acnts.get( quantity.symbol.code().raw(), "no lock balance object found" );

   eosio_assert( from.balance >= (lock_from.locked_balance + quantity), "overdrawn balance for stake action" );

   lock_from_acnts.modify( lock_from, rampayer, [&]( auto& a ) {
      a.locked_balance += quantity;
   });
}

void token::stake(name owner, asset quantity)
{
   require_auth( owner );
   inline_stake(owner, quantity, owner); // use owner as payer 
}

void token::autostake(name owner, asset quantity) 
{
   require_auth( _self );
   inline_stake(owner, quantity, same_payer); // keep payer as before
}

void token::unstake(name owner, asset quantity)
{
   require_auth( owner );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must unstake positive quantity" );

   lock_accounts lock_from_acnts( _self, owner.value );

   const auto& sym_code_raw = quantity.symbol.code().raw();

   const auto& lock_from = lock_from_acnts.get( sym_code_raw, "no balance object found" );
   eosio_assert( lock_from.locked_balance >= quantity, "overdrawn locked balance" );
   // lock_from_acnts.modify( lock_from, owner, [&]( auto& a ) {
   //    a.locked_balance = a.locked_balance;
   // });   

   stats statstable( _self, sym_code_raw);
   const auto& st = statstable.get( sym_code_raw , "symbol does not exist");

   refunds_table refunds_tbl( _self, owner.value );

   uint64_t auto_index = refunds_tbl.available_primary_key();

   asset unstaking_amount = collect_refund(owner, quantity.symbol, auto_index);

   eosio_assert( lock_from.locked_balance >= (unstaking_amount + quantity), "overdrawn locked balance" );

   refunds_tbl.emplace( owner, [&]( refund_request& r ) {
      r.index = auto_index;
      r.owner = owner;
      r.available_time = current_time_point() + seconds(st.refund_delay);
      r.amount = quantity;
   });

   // defer to call refund
   eosio::transaction out;
   //self needs eosio.code permission
   out.actions.emplace_back( permission_level{_self, "active"_n}, _self, "autorefund"_n, std::make_tuple(owner, auto_index) );
   out.delay_sec = st.refund_delay;
   uint128_t sender_id = SENDER_ID(owner.value, auto_index);
   cancel_deferred( sender_id );
   out.send( sender_id, owner, false );
}

asset token::collect_refund(name owner, const symbol& symbol, uint64_t auto_index)
{
   refunds_table refunds_tbl( _self, owner.value );
   //iterate to add up all refund requests
   asset unstaking_amount = asset{0, symbol};
   for (uint64_t i = 0; i < auto_index; i++) {
        auto req = refunds_tbl.find(i);
        if (req != refunds_tbl.end()) {
           if (req->amount.symbol.code().raw() == symbol.code().raw()) {
               unstaking_amount += req->amount;
           }
        }
   }
   return unstaking_amount;
}

void token::inline_refund(name owner, name rampayer, uint64_t index)
{
   refunds_table refunds_tbl( _self, owner.value );
   auto req = refunds_tbl.find( index );
   eosio_assert( req != refunds_tbl.end(), "refund request not found" );
   eosio_assert( req->available_time <= current_time_point(), "refund is not available yet" );

   asset quantity = req->amount;

   lock_accounts lock_from_acnts( _self, owner.value );

   const auto& lock_from = lock_from_acnts.get( quantity.symbol.code().raw(), "no balance object found" );
   eosio_assert( lock_from.locked_balance >= quantity, "overdrawn locked balance" );

   lock_from_acnts.modify( lock_from, rampayer, [&]( auto& a ) {
      a.locked_balance -= quantity;
   });

   refunds_tbl.erase( req );
}

void token::autorefund(name owner, uint64_t index) 
{
   require_auth( _self );
   inline_refund(owner, same_payer, index); // keep payer as before
}

void token::refund(name caller, name owner, uint64_t index) 
{
   require_auth( caller );
   inline_refund(owner, same_payer, index); // keep payer as before
}

void token::cancelunstake(name owner, uint64_t index)
{
   require_auth( owner );

   uint128_t sender_id = SENDER_ID(owner.value, index);
   cancel_deferred( sender_id );

   refunds_table refunds_tbl( _self, owner.value );
   auto req = refunds_tbl.find( index );
   eosio_assert( req != refunds_tbl.end(), "refund request not found" );
   refunds_tbl.erase( req );
}

void token::sub_balance( name owner, asset value , bool use_locked_balance) 
{
   accounts from_acnts( _self, owner.value );
   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found in accounts" );

   lock_accounts lock_from_acnts( _self, owner.value );
   const auto& lock_from = lock_from_acnts.get( value.symbol.code().raw(), "no balance object found in lock_accounts" );

   if(use_locked_balance) {
      eosio_assert( lock_from.locked_balance >= value, "sub_balance: lock_from.locked_balanc overdrawn balance" );
   }else {
      eosio_assert( from.balance >= ( value + lock_from.locked_balance), "sub_balance: from.balance overdrawn balance" );
   }

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
   });
}

void token::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });

      // lock balance record init
      lock_accounts lock_to_acnts( _self, owner.value );
      auto lock_to = lock_to_acnts.find( value.symbol.code().raw() );
      if( lock_to == lock_to_acnts.end() ) {
         lock_to_acnts.emplace( ram_payer, [&]( auto& a ){
            a.locked_balance = asset{0, value.symbol};
         });
      }

   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }

   lock_accounts lock_acnts( _self, owner.value );
   auto lock_to = lock_acnts.find( sym_code_raw );
   if( lock_to == lock_acnts.end() ) {
      lock_acnts.emplace( ram_payer, [&]( auto& a ){
         a.locked_balance = asset{0, symbol};
      });
   }
}

void token::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );

   lock_accounts lock_acnts( _self, owner.value );
   auto lock_it = lock_acnts.find( symbol.code().raw() );
   eosio_assert( lock_it != lock_acnts.end(), "Lock Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( lock_it->locked_balance.amount == 0, "Cannot close because the balance is not zero." );
   lock_acnts.erase( lock_it );

}

void token::setdelay(const symbol& symbol, uint64_t t)
{
   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist." );
   eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch." );

   require_auth( st.issuer );
   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.refund_delay = t;
   });
}

void token::settransfee(const symbol& symbol, uint64_t r, name receiver)
{
   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist." );
   eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch." );

   require_auth( st.issuer );
   
   eosio_assert( r >=0 && r <= 100, "transfer fee is invalid.");

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.transfer_fee_ratio = r;
      s.fee_receiver = receiver;
   });
}

EOSIO_DISPATCH( token, (create)(issue)(transfer)(open)(close)(retire)(stake)(unstake)(cancelunstake)(refund)(autorefund)(setdelay)(settransfee)(autostake))