const { describe } = require('riteway')
const { eos, names } = require('../scripts/helper')
const { equals } = require('ramda')

const publicKey = 'EOS7iYzR2MmQnGga7iD2rPzvm5mEFXx6L1pjFTQYKRtdfDcG9NTTU'

const { accounts, harvest, token, application, firstuser, seconduser } = names

describe('account creation', async assert => {
  const contract = await eos.contract(accounts)

  const newuser = 'newusername'

  await contract.addrequest(application, newuser, publicKey, publicKey, { authorization: `${application}@active` })
  await contract.fulfill(application, newuser, { authorization: `${accounts}@owner` })

  const { required_auth: { keys } } =
    (await eos.getAccount(newuser))
      .permissions.find(p => p.perm_name == 'active')

  assert({
    given: 'created user',
    should: 'have correct public key',
    actual: keys,
    expected: [{
      key: publicKey,
      weight: 1
    }]
  })
})

describe.only('accounts', async assert => {
  const contract = await eos.contract(accounts)
  const thetoken = await eos.contract(token)

  console.log('reset accounts')
  await contract.reset({ authorization: `${accounts}@active` })

  console.log('add application')
  await contract.addapp(application, { authorization: `${accounts}@active` })

  try {
    console.log(`create account ${firstuser}`)
    await contract.addrequest(application, firstuser, publicKey, publicKey, { authorization: `${application}@active` })
    await contract.fulfill(application, firstuser, { authorization: `${accounts}@owner` })
  } catch (err) {
    console.log(`${firstuser} account already exists`)
  }

  try {
    console.log(`create account ${seconduser}`)
    await contract.addrequest(application, seconduser, publicKey, publicKey, { authorization: `${application}@active` })
    await contract.fulfill(application, seconduser, { authorization: `${accounts}@owner` })
  } catch (err) {
    console.log(`${seconduser} account already exists`)
  }

  console.log('join users')
  await contract.adduser(firstuser, 'First user', { authorization: `${accounts}@active` })
  await contract.joinuser(seconduser, { authorization: `${accounts}@active` })

  console.log('add referral')
  await contract.addref(firstuser, seconduser, { authorization: `${accounts}@active` })

  const users = await eos.getTableRows({
    code: accounts,
    scope: accounts,
    table: 'users',
    json: true,
  })

  const apps = await eos.getTableRows({
    code: accounts,
    scope: accounts,
    table: 'apps',
    json: true,
  })

  const refs = await eos.getTableRows({
    code: accounts,
    scope: accounts,
    table: 'refs',
    json: true
  })

  const now = new Date() / 1000

  const firstTimestamp = users.rows[0].timestamp

  assert({
    given: 'created user',
    should: 'have correct timestamp',
    actual: Math.abs(firstTimestamp - now) < 5,
    expected: true
  })

  assert({
    given: 'invited user',
    should: 'have row in table',
    actual: refs,
    expected: {
      rows: [{
        referrer: firstuser,
        invited: seconduser
      }],
      more: false
    }
  })

  assert({
    given: 'added app',
    should: 'have row in table',
    actual: apps,
    expected: {
      rows: [{
        account: application
      }],
      more: false
    }
  })


})
