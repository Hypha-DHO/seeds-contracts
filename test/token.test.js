const { describe } = require('riteway')

const { eos, names, getTableRows, getBalance, initContracts, isLocal } = require('../scripts/helper')
const { assert } = require('chai')

const { token, firstuser, seconduser, thirduser, history, accounts, harvest, settings } = names

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms))

const getSupply = async () => {
  const { rows } = await getTableRows({
    code: token,
    scope: 'SEEDS',
    table: 'stat',
    json: true
  })

  return Number.parseInt(rows[0].supply)
}

describe('token.transfer.history', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ token, history, accounts, settings })
  
  const transfer = () => contracts.token.transfer(firstuser, seconduser, '10.0000 SEEDS', ``, { authorization: `${firstuser}@active` })
  
  console.log('configure')
  await contracts.settings.reset({ authorization: `${settings}@active` })

  console.log('reset token')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  console.log('reset history')
  await contracts.history.reset(firstuser, { authorization: `${history}@active` })

  console.log('accounts reset')
  await contracts.accounts.reset({ authorization: `${accounts}@active` })

  console.log('update status')
  await contracts.accounts.adduser(firstuser, '', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(seconduser, '', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.testresident(firstuser, { authorization: `${accounts}@active` })
  await contracts.accounts.testcitizen(seconduser, { authorization: `${accounts}@active` })
  
  console.log('transfer token')
  await transfer()
  await sleep(500)
  
  const { rows } = await getTableRows({
    code: history,
    scope: firstuser,
    table: 'transactions',
    json: true
  })
  delete rows[0].timestamp

  assert({
    given: 'transactions table',
    should: 'have transaction entry',
    actual: rows,
    expected: [{
      id: 0,
      to: seconduser,
      quantity: '10.0000 SEEDS',
    }]
  })

  await transfer()
  await sleep(500)

  const stats = await getTableRows({
    code: token,
    scope: "SEEDS",
    table: 'trxstat',
    json: true
  })

  //console.log("stats: "+JSON.stringify(stats, null, 2))

  assert({
    given: 'transactions',
    should: 'have transaction stat entries',
    actual: stats.rows.filter( (item) => item.account == firstuser || item.account == seconduser),
    expected: [
      {
        "account": "seedsuseraaa",
        "transactions_volume": "20.0000 SEEDS",
        "total_transactions": 2,
        "incoming_transactions": 0,
        "outgoing_transactions": 2
      },
      {
        "account": "seedsuserbbb",
        "transactions_volume": "20.0000 SEEDS",
        "total_transactions": 2,
        "incoming_transactions": 2,
        "outgoing_transactions": 0
      }
    ]
  })


})

describe('token.transfer', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }
  const contracts = await initContracts({ token, harvest })

  console.log('harvest reset')
  await contracts.harvest.reset({ authorization: `${harvest}@active` })

  let limit = 20
  const transfer = (n) => contracts.token.transfer(firstuser, seconduser, '10.0000 SEEDS', `x${n}`, { authorization: `${firstuser}@active` })

  const balances = [await getBalance(firstuser)]

  console.log('reset token stats')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  console.log('plant SEEDS')
  await contracts.token.transfer(firstuser, harvest, '2.0000 SEEDS', '', { authorization: `${firstuser}@active` })

  console.log(`call transfer x${limit} times`)
  while (--limit >= 0) {
    await contracts.token.transfer(firstuser, seconduser, '10.0000 SEEDS', limit + "x", { authorization: `${firstuser}@active` })
  }

  let limitFailWithPlantedBalance = false
  try {
    await transfer()
    console.log('transferred over limit (NOT expected)')
  } catch (err) {
    limitFailWithPlantedBalance = true
    console.log('transfer over limit failed (as expected)')
  }

  balances.push(await getBalance(firstuser))

  console.log('harvest reset')
  await contracts.harvest.reset({ authorization: `${harvest}@active` })

  console.log('reset token stats')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  limit = 7
  console.log(`call transfer x${limit} times`)
  while (--limit >= 0) {
    await transfer(limit)
  }

  let limitFailWithNoPlatedBalance = false
  try {
    await transfer()
    console.log('transferred over limit (NOT expected)')
  } catch (err) {
    limitFailWithNoPlatedBalance = true
    console.log('transfer over limit failed (as expected)')
  }

  assert({
    given: 'token.transfer called',
    should: 'decrease user balance',
    actual: balances[1],
    expected: balances[0] - 202
  })

  assert({
    given: 'Limit of allowed transactions with planted seeds',
    should: 'fail when limit reached',
    actual: limitFailWithPlantedBalance,
    expected: true
  })

  assert({
    given: 'Limit of allowed transactions with no planted seeds',
    should: 'fail when limit reached',
    actual: limitFailWithNoPlatedBalance,
    expected: true
  })
})

describe('token.burn', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ token })

  const balances = []
  const supply = []

  balances.push(await getBalance(firstuser))
  supply.push(await getSupply())

  await contracts.token.burn(firstuser, '10.0000 SEEDS', { authorization: `${firstuser}@active` })

  balances.push(await getBalance(firstuser))
  supply.push(await getSupply())

  assert({
    given: 'token.burn called',
    should: 'decrease user balance',
    actual: balances[1],
    expected: balances[0] - 10
  })

  assert({
    given: 'token.burn called',
    should: 'decrease total supply',
    actual: supply[1],
    expected: supply[0] - 10
  })
})

describe('token calculate circulating supply', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ token })
  
  console.log('reset token stats')
  await contracts.token.resetweekly({ authorization: `${token}@active` })
  
  console.log('update circulating')
  await contracts.token.updatecirc({ authorization: `${token}@active` })
  
  console.log('transfer token')
  await contracts.token.transfer(firstuser, seconduser, '10.0000 SEEDS', `cc1`, { authorization: `${firstuser}@active` })
  
  const { rows } = await getTableRows({
    code: token,
    scope: token,
    table: 'circulating',
    json: true
  })
  
  console.log("circulating: "+JSON.stringify(rows, null, 2))

  assert({
    given: 'update circulating',
    should: 'have token circulating number',
    actual: rows.length,
    expected: 1
  })
})

describe('token.resetweekly', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ token, settings, accounts })
  
  console.log('configure')
  await contracts.settings.configure("batchsize", 2, { authorization: `${settings}@active` })

  console.log('accounts reset')
  await contracts.accounts.reset({ authorization: `${accounts}@active` })

  console.log('reset token')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  await sleep(10 * 1000)

  console.log('update status')
  await contracts.accounts.adduser(firstuser, '', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(seconduser, '', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(thirduser, '', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.testresident(firstuser, { authorization: `${accounts}@active` })
  await contracts.accounts.testcitizen(seconduser, { authorization: `${accounts}@active` })
  await contracts.accounts.testcitizen(thirduser, { authorization: `${accounts}@active` })

  await contracts.token.transfer(firstuser, seconduser, '10.0000 SEEDS', ``, { authorization: `${firstuser}@active` })
  await contracts.token.transfer(seconduser, thirduser, '10.0000 SEEDS', ``, { authorization: `${seconduser}@active` })
  await contracts.token.transfer(thirduser, seconduser, '10.0000 SEEDS', ``, { authorization: `${thirduser}@active` })
  

  let balancesBefore = await getTableRows({
    code: token,
    scope: 'SEEDS',
    table: 'trxstat',
    json: true
  })

  balancesBefore = balancesBefore.rows.map(row => row.outgoing_transactions)
 
  console.log('reset token')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  await sleep(10 * 1000)

  let balancesAfter = await getTableRows({
    code: token,
    scope: 'SEEDS',
    table: 'trxstat',
    json: true
  })

  balancesAfter = balancesAfter.rows.map(row => row.outgoing_transactions)

  await contracts.settings.reset({ authorization: `${settings}@active` })

  assert({
    given: 'called resetweekly',
    should: 'have trxstat table entry',
    actual: balancesBefore.splice(0, 3),
    expected: [1, 1, 1]
  })

  assert({
    given: 'called resetweekly',
    should: 'reset trxstat table',
    actual: balancesAfter.splice(0, 3),
    expected: [0, 0, 0]
  })

})

