/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <../include/seeds.token.hpp>

namespace eosio {

void token::create( const name&   issuer,
                    const asset&  initial_supply )
{
    require_auth( get_self() );

    auto sym = initial_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( initial_supply.is_valid(), "invalid supply");
    check( initial_supply.amount > 0, "max-supply must be positive");

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = initial_supply.symbol;
       s.initial_supply  = initial_supply;
       s.issuer        = issuer;
    });
}


void token::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;
    check( to == st.issuer, "tokens can only be issued to issuer account" );

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}

void token::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void token::burn( const name& from, const asset& quantity )
{
  require_auth(from);

  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");

  stats statstable(get_self(), sym.code().raw());
  auto sitr = statstable.find(sym.code().raw());

  sub_balance(from, quantity);

  statstable.modify(sitr, from, [&](auto& stats) {
    stats.supply -= quantity;
  });
}

void token::migrateall()
{
  require_auth(get_self());
 
  name old_token_account = name("seedstokennx"); 
  name gift_account = name("gift.seeds");

  user_tables users(contracts::accounts, contracts::accounts.value);

  asset total_distributed(0, seeds_symbol);

  auto uitr = users.begin();
  
  while (uitr != users.end()) {
    name user_account = uitr->account;
    
    accounts user_old_balances(old_token_account, user_account.value);
    
    accounts user_new_balances(get_self(), user_account.value);
    
    auto oitr = user_old_balances.find(seeds_symbol.code().raw());
    
    if (oitr != user_old_balances.end()) {
      asset user_balance = oitr->balance;
      
      auto nitr = user_new_balances.find(seeds_symbol.code().raw());
      
      if (nitr == user_new_balances.end()) {
        user_new_balances.emplace(get_self(), [&](auto& user) {
          user.balance = user_balance;
        });
        
        total_distributed += user_balance;
      }
    }

    uitr++;
  }
  
  accounts gift_balances(get_self(), gift_account.value);

  auto gitr = gift_balances.find(seeds_symbol.code().raw());
  
  gift_balances.modify(gitr, get_self(), [&](auto& gift) {
    gift.balance -= total_distributed;
  });  
}

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    // check_limit(from);

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
    
    save_transaction(from, to, quantity, memo);

    // update_stats( from, to, quantity );
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::save_transaction(name from, name to, asset quantity, string memo) {
  if (!is_account(contracts::accounts) || !is_account(contracts::history)) {
    // Before our accounts are created, don't record anything
    return;
  }
  
  action(
    permission_level{contracts::history, "active"_n},
    contracts::history, 
    "trxentry"_n,
    std::make_tuple(from, to, quantity, memo)
  ).send();

}

void token::check_limit(const name& from) {
  user_tables users(contracts::accounts, contracts::accounts.value);
  auto uitr = users.find(from.value);

  if (uitr == users.end()) {
    return;
  }

  name status = uitr->status;

  uint64_t limit = 10;
  if (status == "resident"_n) {
    limit = 50;
  } else if (status == "citizen"_n) {
    limit = 100;
  }

  transaction_tables transactions(get_self(), seeds_symbol.code().raw());
  auto titr = transactions.find(from.value);
  uint64_t current = titr->outgoing_transactions;

  check(current < limit, "too many outgoing transactions");
}

void token::resetweekly() {
  require_auth(get_self());
  
  auto sym_code_raw = seeds_symbol.code().raw();

  transaction_tables transactions(get_self(), sym_code_raw);

  auto titr = transactions.begin();
  while (titr != transactions.end()) {
    transactions.modify(titr, get_self(), [&](auto& user) {
      user.incoming_transactions = 0;
      user.outgoing_transactions = 0;
      user.total_transactions = 0;
      user.transactions_volume = asset(0, seeds_symbol);
    });
    titr++;
  }

  transaction trx{};
  trx.actions.emplace_back(
    permission_level(_self, "active"_n),
    _self,
    "resetweekly"_n,
    std::make_tuple()
  );
  trx.delay_sec = 3600 * 7;
  trx.send(eosio::current_time_point().sec_since_epoch(), _self);
}

void token::update_stats( const name& from, const name& to, const asset& quantity ) {
    auto sym_code_raw = quantity.symbol.code().raw();

    transaction_tables transactions(get_self(), sym_code_raw);
    user_tables users(contracts::accounts, contracts::accounts.value);

    auto fromitr = transactions.find(from.value);
    auto toitr = transactions.find(to.value);
    
    auto fromuser = users.find(from.value);
    auto touser = users.find(to.value);
    
    if (fromuser == users.end() || touser == users.end()) {
      return;
    }

    if (fromitr == transactions.end()) {
      transactions.emplace(get_self(), [&](auto& user) {
        user.account = from;
        user.transactions_volume = quantity;
        user.total_transactions = 1;
        user.incoming_transactions = 0;
        user.outgoing_transactions = 1;
      });
    } else {
      transactions.modify(fromitr, get_self(), [&](auto& user) {
          user.transactions_volume += quantity;
          user.outgoing_transactions += 1;
          user.total_transactions += 1;
      });
    }

    if (toitr == transactions.end()) {
      transactions.emplace(get_self(), [&](auto& user) {
        user.account = to;
        user.transactions_volume = quantity;
        user.total_transactions = 1;
        user.incoming_transactions = 1;
        user.outgoing_transactions = 0;
      });
    } else {
      transactions.modify(toitr, get_self(), [&](auto& user) {
        user.transactions_volume += quantity;
        user.total_transactions += 1;
        user.incoming_transactions += 1;
      });
    }
}

void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void token::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::token, (create)(issue)(transfer)(open)(close)(retire)(burn)(resetweekly)(migrateall) )
