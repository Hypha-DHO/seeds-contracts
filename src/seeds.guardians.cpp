#include <seeds.guardians.hpp>

void guardians::reset() {
    require_auth(get_self());

    auto gitr = guardians.begin();
    while (gitr != guardians.end()) {
        gitr = guardians.erase(gitr);
    }

    auto ritr = recovers.begin();
    while (ritr != recovers.end()) {
        ritr = recovers.erase(ritr);
    }
}

void guardians::init(name user_account, vector<name> guardian_accounts) {
    require_auth(user_account);
    
    auto gitr = guardians.find(user_account.value);

    assert(gitr == guardians.end(),
        "account " + user_account.to_string() + " already have guardians");

    assert(guardian_accounts.size() == 3,
        "provided " + guardian_accounts.size() + " guardians, but needed only 3 guardians");

    for (std::size_t i = 0; i < guardian_accounts.size(); ++i) {
        assert(is_seeds_user(guardian_accounts[i]),
            "guardian " + guardian_accounts[i].to_string() + " is not a seeds user");
    }

    guardians.emplace(get_self(), [&](auto& item) {
        item.account = user_account;
        item.guardians = guardian_accounts;
    });
}

void guardians::cancel(name user_account) {
    require_auth(user_account);

    auto gitr = guardians.find(user_account.value);

    assert(gitr != guardians.end(),
        "account " + user_account.to_string() + " does not have guardians");

    guardians.erase(gitr);

    auto ritr = recovers.find(user_account.value);

    if (ritr != recovers.end()) {
        recovers.erase(ritr);
    }
}

void guardians::recover(name guardian_account, name user_account, string new_public_key) {
    require_auth(guardian_account);

    auto gitr = guardians.find(user_account.value);

    assert(gitr != guardians.end(),
        "account " + user_account.to_string() + " does not have guardians");

    bool is_user_guardian = false;

    for (std::size_t i = 0; i < gitr->guardians.size(); ++i) {
        if (gitr->guardians[i] == guardian_account) {
            is_user_guardian = true;
        }
    }

    assert(is_user_guardian == true,
        "account " + guardian_account.to_string() +
        " is not a guardian for " + user_account.to_string());

    auto ritr = recovers.find(user_account.value);

    if (ritr == recovers.end()) {
        recovers.emplace(get_self(), [&](auto& item) {
            item.account = user_account;
            item.guardians = vector(guardian_account);
            item.public_key = new_public_key;
        });
    } else {
        if (ritr->public_key.compare(new_public_key) == 0) {
            bool is_guardian_recovering = false;

            for (std::size_t i = 0; i < ritr->guardians.size(); ++i) {
                if (ritr->guardians[i] == guardian_account) {
                    is_guardian_recovering = true;
                }
            }

            assert (is_guardian_recovering == false,
                "guardian " + guardian_account.to_string() + " already recovering " + user_account.to_string());        

            recovers.modify(get_self(), [&](auto& item) {
                item.guardians = vector(item.guardians, guardian_account);
            });          
        } else {
            recovers.modify(get_self(), [&](auto& item) {
                item.guardians = vector(guardian_account);
                item.public_key = new_public_key;
            });
        }
    }

    int required_guardians = gitr->guardians.size();
    int signed_guardians = ritr->guardians.size();

    if ((required_guardians == 3 && signed_guardians >= 2)
        || (required_guardians > 3 && signed_guardians >= 3)) {

        change_account_permission(user_account, new_public_key);
    } 
}

void change_account_permission(account user_account, string public_key) {
    authority newauth;
    newauth.threshold = 1;
    key_weight kweight = {key: public_key, weight: 1};
    newauth.keys.emplace_back(kweight);
    
    action(
        permission_level{get_self(), "owner"_n},
        "eosio"_n, "updateauth"_n,
        make_tuple(
            user_account,
            "active"_n,
            "owner"_n,
            newauth,
        )
    ).send();
}

bool guardians::is_seeds_user(name account) {
    DEFINE_USER_TABLE;
    DEFINE_USER_TABLE_MULTI_INDEX;
    user_tables users(contracts::accounts, contracts::accounts.value);

    auto uitr = users.find(account.value);
    return uitr != users.end();
}