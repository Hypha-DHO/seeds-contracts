#include <seeds.invites.hpp>

void invites::create_account(name account, string publicKey) {
  if (is_account(account)) return;

  authority auth = keystring_authority(publicKey);

  action(
    permission_level{_self, "owner"_n},
    "eosio"_n, "newaccount"_n,
    make_tuple(_self, account, auth, auth)
  ).send();

  action(
    permission_level{_self, "owner"_n},
    "eosio"_n, "buyram"_n,
    make_tuple(_self, account, asset(21000, network_symbol))
  ).send();

  action(
    permission_level{_self, "owner"_n},
    "eosio"_n, "delegatebw"_n,
    make_tuple(_self, account, asset(100, network_symbol), asset(1000, network_symbol), 0)
  ).send();
}

void invites::add_user(name account) {
  string nickname("");

  action(
    permission_level{"seedsaccnts3"_n, "active"_n},
    "seedsaccnts3"_n, "adduser"_n,
    make_tuple(account, nickname)
  ).send();
}

void invites::transfer_seeds(name account, asset quantity) {
  string memo("");

  action(
    permission_level{_self, "active"_n},
    "seedstoken12"_n, "transfer"_n,
    make_tuple(_self, account, quantity, memo)
  ).send();
}

void invites::plant_seeds(asset quantity) {
  string memo("");

  action(
    permission_level{_self, "active"_n},
    "seedstoken12"_n, "transfer"_n,
    make_tuple(_self, "seedshrvst11"_n, quantity, memo)
  ).send();
}

void invites::sow_seeds(name account, asset quantity) {
  action(
    permission_level{_self, "active"_n},
    "seedshrvst11"_n, "sow"_n,
    make_tuple(_self, account, quantity)
  ).send();
}

void invites::reset() {
  require_auth(get_self());

  auto sitr = sponsors.begin();

  while (sitr != sponsors.end()) {
    sitr = sponsors.erase(sitr);
  }
}

void invites::send(name from, name to, asset quantity, string memo) {
  if (to == get_self()) {
    auto sitr = sponsors.find(from.value);

    if (sitr == sponsors.end()) {
      sponsors.emplace(_self, [&](auto& sponsor) {
        sponsor.account = from;
        sponsor.balance = quantity;
      });
    } else {
      sponsors.modify(sitr, _self, [&](auto& sponsor) {
        sponsor.balance += quantity;
      });
    }
  }
}

void invites::accept(name sponsor, name account, string publicKey, asset quantity) {
  require_auth(get_self());

  auto sitr = sponsors.find(sponsor.value);
  check(sitr != sponsors.end(), "sponsor not found");
  check(sitr->balance >= quantity, "not enough balance");

  sponsors.modify(sitr, _self, [&](auto& sponsor) {
    sponsor.balance -= quantity;
  });

  asset transfer_quantity = asset(quantity.amount - sow_amount, seeds_symbol);
  asset sow_quantity = asset(sow_amount, seeds_symbol);

  create_account(account, publicKey);
  add_user(account);
  transfer_seeds(account, transfer_quantity);
  plant_seeds(sow_quantity);
  sow_seeds(account, sow_quantity);
}