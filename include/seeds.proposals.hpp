#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/transaction.hpp>
#include <seeds.token.hpp>

using namespace eosio;
using std::string;

CONTRACT proposals : public contract {
  public:
      using contract::contract;
      proposals(name receiver, name code, datastream<const char*> ds)
        : contract(receiver, code, ds),
          props(receiver, receiver.value),
          voice(receiver, receiver.value),
          config(name("seedsettings"), name("seedsettings").value),
          users(name("seedsaccnts3"), name("seedsaccnts3").value)
          {}

      ACTION reset();

      ACTION create(name creator, name recipient, asset quantity, string title, string summary, string description, string image, string url);

      ACTION update(uint64_t id, string title, string summary, string description, string image, string url);

      ACTION stake(name from, name to, asset quantity, string memo);

      ACTION addvoice(name user, uint64_t amount);

      ACTION favour(name user, uint64_t id, uint64_t amount);

      ACTION against(name user, uint64_t id, uint64_t amount);

      ACTION onperiod();
  private:
      symbol seeds_symbol = symbol("SEEDS", 4);

      void check_user(name account);
      void check_asset(asset quantity);
      void deposit(asset quantity);
      void withdraw(name account, asset quantity);
      void burn(asset quantity);

      TABLE config_table {
          name param;
          uint64_t value;
          uint64_t primary_key()const { return param.value; }
      };

      TABLE proposal_table {
          uint64_t id;
          name creator;
          name recipient;
          asset quantity;
          asset staked;
          bool executed;
          uint64_t total;
          uint64_t favour;
          uint64_t against;
          string title;
          string summary;
          string description;
          string image;
          string url;
          name status;
          uint64_t creation_date;
          uint64_t primary_key()const { return id; }
      };

      TABLE user_table {
          name account;
          uint64_t primary_key()const { return account.value; }
      };

      TABLE vote_table {
          name account;
          uint64_t primary_key()const { return account.value; }
      };

      TABLE voice_table {
        name account;
        uint64_t balance;
        uint64_t primary_key()const { return account.value; }
      };

      typedef eosio::multi_index<"props"_n, proposal_table> proposal_tables;
      typedef eosio::multi_index<"votes"_n, vote_table> votes_tables;
      typedef eosio::multi_index<"config"_n, config_table> config_tables;
      typedef eosio::multi_index<"users"_n, user_table> user_tables;
      typedef eosio::multi_index<"voice"_n, voice_table> voice_tables;

      config_tables config;
      proposal_tables props;
      user_tables users;
      voice_tables voice;
};

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  if (action == name("transfer").value && code == "seedstoken12"_n.value) {
      execute_action<proposals>(name(receiver), name(code), &proposals::stake);
  } else if (code == receiver) {
      switch (action) {
        EOSIO_DISPATCH_HELPER(proposals, (reset)(create)(update)(addvoice)(favour)(against)(onperiod))
      }
  }
}