#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <contracts.hpp>
#include <utils.hpp>
#include <tables/config_table.hpp>

using namespace eosio;
using std::string;


CONTRACT scheduler : public contract {

    public:
        using contract::contract;
        scheduler(name receiver, name code, datastream<const char*> ds)
        :contract(receiver, code, ds),
        operations(receiver, receiver.value),
        moonphases(receiver, receiver.value),
        test(receiver, receiver.value),
        config(contracts::settings, contracts::settings.value)
        {}

        ACTION reset();

        ACTION updateops();

        ACTION execute();

        // specify start time any time in the future, or use 0 for "now"
        ACTION configop(name id, name action, name contract, uint64_t period, uint64_t starttime);

        ACTION removeop(name id);

        ACTION pauseop(name id, uint8_t pause);

        ACTION confirm(name operation);
        
        ACTION stop();
        
        ACTION start();

        ACTION moonphase(uint64_t timestamp, string phase_name, string eclipse);

        ACTION test1();

        ACTION test2();

        ACTION testexec(name op);

    private:
        void exec_op(name id, name contract, name action);
        void cancel_exec();
        void reset_aux(bool destructive);
        bool should_preserve_op(name op_id) {
            return 
                op_id == "exch.period"_n || 
                op_id == "tokn.resetw"_n;
        }

        TABLE operations_table {
            name id;
            name operation;
            name contract;
            uint8_t pause;
            uint64_t period;
            uint64_t timestamp;

            uint64_t primary_key() const { return id.value; }
            uint64_t by_timestamp() const { return timestamp; }
        };

        TABLE moon_phases_table {
            uint64_t timestamp;
            time_point time;
            string phase_name;
            string eclipse;

            uint64_t primary_key() const { return timestamp; }
        };

        TABLE test_table {
            name param;
            uint64_t value;
            uint64_t primary_key() const { return param.value; }
        };

        DEFINE_CONFIG_TABLE
        
        DEFINE_CONFIG_TABLE_MULTI_INDEX

        typedef eosio::multi_index < "operations"_n, operations_table,
            indexed_by<"bytimestamp"_n, const_mem_fun<operations_table, uint64_t, &operations_table::by_timestamp>>
        > operations_tables;

        typedef eosio::multi_index <"moonphases"_n, moon_phases_table> moon_phases_tables;

        typedef eosio::multi_index <"test"_n, test_table> test_tables;

        name seconds_to_execute = "secndstoexec"_n;

        operations_tables operations;
        config_tables config;
        test_tables test;
        moon_phases_tables moonphases;

        bool is_ready_to_execute(name operation);
};
