#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <contracts.hpp>
#include <tables.hpp>
#include <tables/rep_table.hpp>
#include <tables/size_table.hpp>
#include <tables/cbs_table.hpp>
#include<tables/user_table.hpp>
#include <utils.hpp>

using namespace eosio;
using std::string;

CONTRACT accounts : public contract {
  public:
      using contract::contract;
      accounts(name receiver, name code, datastream<const char*> ds)
        : contract(receiver, code, ds),
          users(receiver, receiver.value),
          refs(receiver, receiver.value),
          cbs(receiver, receiver.value),
          cbs2(receiver, receiver.value),
          reqvouch(receiver, receiver.value),
          rep(receiver, receiver.value),
          sizes(receiver, receiver.value),
          balances(contracts::harvest, contracts::harvest.value),
          config(contracts::settings, contracts::settings.value)
          {}

      ACTION reset();

      ACTION adduser(name account, string nickname, name type);

      ACTION makeresident(name user);

      ACTION makecitizen(name user);

      ACTION testcitizen(name user);

      ACTION genesis(name user);

      ACTION update(name user, name type, string nickname, string image, string story, string roles, string skills, string interests);

      ACTION addref(name referrer, name invited);

      ACTION invitevouch(name referrer, name invited);

      ACTION addrep(name user, uint64_t amount);

      ACTION subrep(name user, uint64_t amount);

      ACTION punish(name account);

      ACTION requestvouch(name account, name sponsor);

      ACTION vouch(name sponsor, name account);

      //ACTION migraterep(uint64_t account, uint64_t cycle, uint64_t chunksize); // MIGRATION - REMOVE
      //ACTION resetrep();  // MIGRATION - REMOVE

      ACTION rankreps();
      ACTION rankrep(uint64_t start_val, uint64_t chunk, uint64_t chunksize);

      ACTION rankcbss();
      ACTION rankcbs(uint64_t start_val, uint64_t chunk, uint64_t chunksize);

      ACTION migratecbs(); // MIGRATION - REMOVE
      ACTION clearcbs(); // MIGRATION - REMOVE

      ACTION testresident(name user);
      ACTION testvisitor(name user);
      ACTION testremove(name user);
      ACTION testsetrep(name user, uint64_t amount);
      ACTION testsetcbs(name user, uint64_t amount);
      ACTION testreward();

  private:
      symbol seeds_symbol = symbol("SEEDS", 4);
      symbol network_symbol = symbol("TLOS", 4);

      const name individual = "individual"_n;
      const name organization = "organisation"_n;

      const name not_found = ""_n;
      const name cbp_reward_resident = "refcbp1.ind"_n;
      const name cbp_reward_citizen = "refcbp2.ind"_n;

      const name reputation_reward_resident = "refrep1.ind"_n;
      const name reputation_reward_citizen = "refrep2.ind"_n;

      const name individual_seeds_reward_resident = "refrwd1.ind"_n;
      const name individual_seeds_reward_citizen = "refrwd2.ind"_n;

      const name org_seeds_reward_resident = "refrwd1.org"_n;
      const name org_seeds_reward_citizen = "refrwd2.org"_n;

      const name ambassador_seeds_reward_resident = "refrwd1.amb"_n;
      const name ambassador_seeds_reward_citizen = "refrwd2.amb"_n;

      const name max_vouch_points = "maxvouch"_n;

      const name resident_vouch_points = "res.vouch"_n;
      const name citizen_vouch_points = "cit.vouch"_n;

      void buyaccount(name account, string owner_key, string active_key);
      void check_user(name account);
      void rewards(name account, name new_status);
      void vouchreward(name account);
      void refreward(name account, name new_status);
      void send_reward(name beneficiary, asset quantity);
      void updatestatus(name account, name status);
      void _vouch(name sponsor, name account);
      void history_add_resident(name account);
      void history_add_citizen(name account);
      name find_referrer(name account);
      void send_addrep(name user, uint64_t amount);
      void send_subrep(name user, uint64_t amount);
      void send_to_escrow(name fromfund, name recipient, asset quantity, string memo);
      uint64_t countrefs(name user);
      uint64_t rep_score(name user);
      void add_rep_item(name account, uint64_t reputation);
      void size_change(name id, int delta);
      void size_set(name id, uint64_t newsize);

      DEFINE_USER_TABLE

      DEFINE_USER_TABLE_MULTI_INDEX

      DEFINE_REP_TABLE

      DEFINE_REP_TABLE_MULTI_INDEX

      DEFINE_SIZE_TABLE

      DEFINE_SIZE_TABLE_MULTI_INDEX

      DEFINE_CBS_TABLE

      DEFINE_CBS_TABLE_MULTI_INDEX

      TABLE ref_table {
        name referrer;
        name invited;

        uint64_t primary_key() const { return invited.value; }
        uint64_t by_referrer()const { return referrer.value; }
      };

      TABLE cbs2_table {
        name account;
        uint64_t community_building_score;

        uint64_t primary_key() const { return account.value; }
        uint64_t by_cbs()const { return community_building_score; }
      };

      TABLE vouch_table {
        name account;
        name sponsor;
        uint64_t reps;

        uint64_t primary_key() const { return sponsor.value; }
        uint64_t by_account()const { return account.value; }

      };

      TABLE req_vouch_table {
        uint64_t id;
        name account;
        name sponsor;

        uint64_t primary_key() const { return id; }
        uint64_t by_account()const { return account.value; }
        uint64_t by_sponsor()const { return sponsor.value; }
      };

      TABLE config_table {
        name param;
        uint64_t value;
        uint64_t primary_key() const { return param.value; }
      };
    
    typedef eosio::multi_index<"refs"_n, ref_table,
      indexed_by<"byreferrer"_n,const_mem_fun<ref_table, uint64_t, &ref_table::by_referrer>>
    > ref_tables;
    
    typedef eosio::multi_index<"vouch"_n, vouch_table,
      indexed_by<"byaccount"_n,
      const_mem_fun<vouch_table, uint64_t, &vouch_table::by_account>>
    > vouch_tables;

    typedef eosio::multi_index<"reqvouch"_n, req_vouch_table,
      indexed_by<"byaccount"_n,
      const_mem_fun<req_vouch_table, uint64_t, &req_vouch_table::by_account>>,
      indexed_by<"bysponsor"_n,
      const_mem_fun<req_vouch_table, uint64_t, &req_vouch_table::by_sponsor>>
    > req_vouch_tables;

    typedef eosio::multi_index<"balances"_n, tables::balance_table,
        indexed_by<"byplanted"_n,
        const_mem_fun<tables::balance_table, uint64_t, &tables::balance_table::by_planted>>
    > balance_tables;
    balance_tables balances;

    typedef eosio::multi_index<"cbs2"_n, cbs2_table,
      indexed_by<"bycbs"_n,
      const_mem_fun<cbs2_table, uint64_t, &cbs2_table::by_cbs>>
    > cbs2_tables;

    typedef eosio::multi_index <"config"_n, config_table> config_tables;

    cbs_tables cbs;
    cbs2_tables cbs2;
    ref_tables refs;
    req_vouch_tables reqvouch;
    user_tables users;
    rep_tables rep;
    size_tables sizes;

    config_tables config;

    // From token contract
    struct [[eosio::table]] transaction_stats {
      name account;
      asset transactions_volume;
      uint64_t total_transactions;
      uint64_t incoming_transactions;
      uint64_t outgoing_transactions;

      uint64_t primary_key()const { return account.value; }
      uint64_t by_transaction_volume()const { return transactions_volume.amount; }
    };
    
    typedef eosio::multi_index< "trxstat"_n, transaction_stats,
      indexed_by<"bytrxvolume"_n,
      const_mem_fun<transaction_stats, uint64_t, &transaction_stats::by_transaction_volume>>
    > transaction_tables;

};

EOSIO_DISPATCH(accounts, (reset)(adduser)(makeresident)(makecitizen)(update)(addref)(invitevouch)(addrep)
(subrep)(testsetrep)(testcitizen)(genesis)(testresident)(testvisitor)(testremove)(testsetcbs)
(testreward)(punish)(requestvouch)(vouch)
(migratecbs)(clearcbs)
(rankreps)(rankrep)(rankcbss)(rankcbs)
);
