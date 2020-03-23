#include <seeds.organization.hpp>
#include <eosio/system.hpp>


void organization::check_owner(name organization, name owner) {
    require_auth(owner);
    auto itr = organizations.get(organization.value, "The organization does not exist.");
    check(itr.owner == owner, "Only the organization's owner can do that.");
    check_user(owner);
}


void organization::init_balance(name account) {
    auto itr = sponsors.find(account.value);
    if(itr == sponsors.end()){
        sponsors.emplace(_self, [&](auto & nbalance) {
            nbalance.account = account;
            nbalance.balance = asset(0, seeds_symbol);
        });
    }
}

void organization::check_user(name account) {
    auto uitr = users.find(account.value);
    check(uitr != users.end(), "organisation: no user.");
}

int64_t organization::getregenp(int64_t points, name account) {
    auto itr = harvest.find(account.value);
    int64_t result = 0;
    if (itr != harvest.end()) {
        result = itr -> reputation_score;
    }
    return result * points;
}

void organization::deposit(name from, name to, asset quantity, string memo) {
    if(to == _self){
        utils::check_asset(quantity);
        check_user(from);

        init_balance(from);
        init_balance(to);

        auto fitr = sponsors.find(from.value);
        sponsors.modify(fitr, _self, [&](auto & mbalance) {
            mbalance.balance += quantity;
        });

        auto titr = sponsors.find(to.value);
        sponsors.modify(titr, _self, [&](auto & mbalance) {
            mbalance.balance += quantity;
        });
    }
}


// this function is just for testing
/*
ACTION organization::createbalance(name user, asset quantity) {
    
    sponsors.emplace(_self, [&](auto & nbalance) {
        nbalance.account = user;
        nbalance.balance = quantity;
    });
}
*/


ACTION organization::reset() {
    require_auth(_self);

    auto itr = organizations.begin();
    while(itr != organizations.end()) {
        name org = itr -> org_name;
        members_tables members(get_self(), org.value);
        vote_tables votes(get_self(), org.value);

        auto mitr = members.begin();
        while(mitr != members.end()) {
            mitr = members.erase(mitr);
        }

        auto vitr = votes.begin();
        while(vitr != votes.end()){
            vitr = votes.erase(vitr);
        }

        itr = organizations.erase(itr);
    }

    auto bitr = sponsors.begin();
    while(bitr != sponsors.end()){
        bitr = sponsors.erase(bitr);
    }
}


ACTION organization::create(name sponsor, name orgaccount, string orgfullname, string publicKey) 
{
    require_auth(sponsor); // should the sponsor give the authorization? or it should be the contract itself?

    auto bitr = sponsors.find(sponsor.value);
    check(bitr != sponsors.end(), "The sponsor account does not have a balance entry in this contract.");

    auto feeparam = config.get(min_planted.value, "The org.minplant parameter has not been initialized yet.");
    asset quantity(feeparam.value, seeds_symbol);

    check(bitr->balance >= quantity, "The user does not have enough credit to create an organization" + bitr->balance.to_string() + " min: "+quantity.to_string());

    auto orgitr = organizations.find(orgaccount.value);
    check(orgitr == organizations.end(), "This organization already exists.");
    
    auto uitr = users.find(sponsor.value);
    check(uitr != users.end(), "Sponsor is not a Seeds account.");

    create_account(sponsor, orgaccount, orgfullname, publicKey);

    string memo =  "sow "+orgaccount.to_string();

    action(
        permission_level(_self, "active"_n),
        contracts::token,
        "transfer"_n,
        std::make_tuple(_self, contracts::harvest, quantity, memo)
    ).send();


    sponsors.modify(bitr, _self, [&](auto & mbalance) {
        mbalance.balance -= quantity;           
    });

    organizations.emplace(_self, [&](auto & norg) {
        norg.org_name = orgaccount;
        norg.owner = sponsor;
        norg.planted = quantity;
        norg.status = 0;
    });

    addmember(orgaccount, sponsor, sponsor, ""_n);
}

void organization::create_account(name sponsor, name orgaccount, string orgfullname, string publicKey) 
{
    action(
        permission_level{contracts::onboarding, "active"_n},
        contracts::onboarding, "onboardorg"_n,
        make_tuple(sponsor, orgaccount, orgfullname, publicKey)
    ).send();
}

ACTION organization::destroy(name organization, name owner) {
    check_owner(organization, owner);

    auto orgitr = organizations.find(organization.value);
    check(orgitr != organizations.end(), "organisation: the organization does not exist.");

    auto bitr = sponsors.find(owner.value);
    sponsors.modify(bitr, _self, [&](auto & mbalance) {
        mbalance.balance += orgitr -> planted;
    });

    members_tables members(get_self(), organization.value);
    auto mitr = members.begin();
    while(mitr != members.end()){
        mitr = members.erase(mitr);
    }
    
    auto org = organizations.find(organization.value);
    organizations.erase(org);

    // refund(owner, planted); this method could be called if we want to refund as soon as the user destroys an organization
}


ACTION organization::refund(name beneficiary, asset quantity) {
    require_auth(beneficiary);
    
    utils::check_asset(quantity);

    auto itr = sponsors.find(beneficiary.value);
    check(itr != sponsors.end(), "organisation: user has no entry in the balance table.");
    check(itr -> balance >= quantity, "organisation: user has not enough balance.");

    string memo = "refund";

    action(
        permission_level(_self, "active"_n),
        contracts::token,
        "transfer"_n,
        std::make_tuple(_self, beneficiary, quantity, memo)
    ).send();

    auto bitr = sponsors.find(_self.value);
    sponsors.modify(bitr, _self, [&](auto & mbalance) {
        mbalance.balance -= quantity;
    });

    sponsors.modify(itr, _self, [&](auto & mbalance) {
        mbalance.balance -= quantity;
    });
}


ACTION organization::addmember(name organization, name owner, name account, name role) {
    check_owner(organization, owner);
    check_user(account);
    
    members_tables members(get_self(), organization.value);
    members.emplace(_self, [&](auto & nmember) {
        nmember.account = account;
        nmember.role = role;
    });
}


ACTION organization::removemember(name organization, name owner, name account) {
    check_owner(organization, owner);

    auto itr = organizations.find(organization.value);
    check(itr -> owner != account, "Change the organization's owner before removing this account.");

    members_tables members(get_self(), organization.value);
    auto mitr = members.find(account.value);
    members.erase(mitr);
}


ACTION organization::changerole(name organization, name owner, name account, name new_role) {
    check_owner(organization, owner);

    members_tables members(get_self(), organization.value);
    
    auto mitr = members.find(account.value);
    check(mitr != members.end(), "Member does not exist.");

    members.modify(mitr, _self, [&](auto & mmember) {
        mmember.role = new_role;
    });
}


ACTION organization::changeowner(name organization, name owner, name account) {
    check_owner(organization, owner);
    check_user(account);

    auto orgitr = organizations.find(organization.value);

    organizations.modify(orgitr, _self, [&](auto & morg) {
        morg.owner = account;
    });
}


void organization::vote(name organization, name account, int64_t regen) {
    vote_tables votes(get_self(), organization.value);

    auto itr = organizations.find(organization.value);
    check(itr != organizations.end(), "organisation does not exist.");
    
    // enter the vote

    votes.emplace(_self, [&](auto & nvote) {
        nvote.account = account;
        nvote.timestamp = eosio::current_time_point().sec_since_epoch();
        nvote.regen_points = regen;
    });

    // calculate the new total score

    int64_t total_points = 0;
    int64_t median = 0;
    int64_t average = 0;
    uint64_t counter = 0;
    int64_t score = 0;

    auto vitr = votes.begin();
    while(vitr != votes.end()) {
        total_points += vitr -> regen_points;
        counter++;
        average = total_points / counter;
        if ( abs(average - vitr -> regen_points) < abs(average - median)) {
            median = vitr -> regen_points;
        }
        vitr++;
    }

    // As per the constituion
    // "When an org has more than 1000 points, its regenerative score is set to the median of the received votes"
    // "When an org has less than 1000 points, its regen score is 0"
    if (total_points > 1000) {
        score = median;
    }

    organizations.modify(itr, _self, [&](auto & morg) {
        morg.regen = score;
    });

    cal_bio_scores();
}

void organization::cal_bio_scores() {
    auto orgregen = organizations.get_index<"byregen"_n>();
    auto number = std::distance(orgregen.begin(), orgregen.end());

    uint64_t current = 1;

    auto oitr = orgregen.begin();

    while (oitr != orgregen.end()) {
        uint64_t score = (current * 100) / number;

        if (oitr->regen == 0) {
            score = 0;
        }

        auto hitr = scores.find(oitr->org_name.value);

        if (hitr == scores.end()) {
            init_scores(oitr->org_name);
            hitr = scores.find(oitr->org_name.value);
        }

        scores.modify(hitr, _self, [&](auto& item) {
            item.biosphere_score = score;
        });

        current++;
        oitr++;
    }   
}

void organization::init_scores(name account) {
  scores.emplace(_self, [&](auto& item) {
    item.account = account;
    item.biosphere_score = 0;    
    item.transaction_score = 0;
    item.community_building_score = 0;
    item.contribution_score = 0;
  });
}


ACTION organization::addregen(name organization, name account, int64_t points) {
    require_auth(account);
    check_user(account);

    check( points >= -7 , "invalid regen vote: points can't be less than -7");
    check( points <= 7  , "invalid regen vote: points can't be greater than 7");

    vote_tables votes(get_self(), organization.value);

    auto vitr = votes.find(account.value);
    
    if(vitr != votes.end()){
        votes.erase(vitr);
    }
    
    vote(organization, account, getregenp(points, account));
}

ACTION organization::addcbs(name org, uint64_t amount) {
    require_auth(get_self());

    auto citr = orgcbs.find(org.value);

    if (citr == orgcbs.end()) {
        orgcbs.emplace(_self, [&](auto& item) {
            item.account = org;
            item.community_building_score = amount;
        });
    } else {
        orgcbs.modify(citr, _self, [&](auto& item) {
            item.community_building_score += amount;
        });
    }

}


