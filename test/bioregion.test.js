const { describe } = require("riteway")
const { eos, encodeName, names, getTableRows, isLocal, initContracts, createKeypair } = require("../scripts/helper")

const { accounts, harvest, bioregion, token, firstuser, seconduser, thirduser, bank, settings, history, fourthuser } = names

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

const randomAccountNameBDC = () => {
  let length = 8
  var result           = '';
  var characters       = 'abcdefghijklmnopqrstuvwxyz1234';
  var charactersLength = characters.length;
  for ( var i = 0; i < length; i++ ) {
     result += characters.charAt(Math.floor(Math.random() * charactersLength));
  }
  return result + ".bdc";
}

describe("Bioregions General", async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ accounts, bioregion, token, settings })

  const keypair = await createKeypair();

  await contracts.settings.reset({ authorization: `${settings}@active` })

  console.log('bioregion reset')
  await contracts.bioregion.reset({ authorization: `${bioregion}@active` })

  console.log('accounts reset')
  await contracts.accounts.reset({ authorization: `${accounts}@active` })

  console.log('reset token stats')
  await contracts.token.resetweekly({ authorization: `${token}@active` })

  console.log('configure - 1 Seeds feee')
  await contracts.settings.configure("bio.fee", 10000 * 1, { authorization: `${settings}@active` })

  console.log('join users')
  await contracts.accounts.adduser(firstuser, 'first user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(seconduser, 'second user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(thirduser, 'thirduser user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(fourthuser, 'second user', 'individual', { authorization: `${accounts}@active` })

  console.log('transfer fee for a bioregion')
  await contracts.token.transfer(firstuser, bioregion, "1.0000 SEEDS", "Initial supply", { authorization: `${firstuser}@active` })

  const bioname = randomAccountNameBDC()

  const getMembers = async () => {
    return await getTableRows({
      code: bioregion,
      scope: bioregion,
      table: 'members',
      json: true
    })
  }
  const getRoles = async () => {
    return await getTableRows({
      code: bioregion,
      scope: bioname,
      table: 'roles',
      json: true
    })
  }

  console.log('create a bioregion named '+bioname)

  await contracts.bioregion.create(
    firstuser, 
    bioname, 
    'test bio region',
    '{lat:0.0111,lon:1.3232}', 
    1.1, 
    1.23, 
    keypair.public, 
    { authorization: `${firstuser}@active` })

    const bioregions = await getTableRows({
      code: bioregion,
      scope: bioregion,
      table: 'bioregions',
      json: true
    })
    //console.log("bioregions "+JSON.stringify(bioregions, null, 2))
  
    const initialMembers = await getMembers()
    
    var hasNewDomain = false
    try {
      const accountInfo = await eos.getAccount(bioname)
      hasNewDomain = true;
    } catch {
  
    }
  
  console.log('join a bioregion')
  await contracts.bioregion.join(bioname, seconduser, { authorization: `${seconduser}@active` })

  const members = await getMembers()
  //console.log("members "+JSON.stringify(members, null, 2))

  console.log('leave a bioregion')
  await contracts.bioregion.leave(bioname, seconduser, { authorization: `${seconduser}@active` })

  const membersAfter = await getMembers()

  //console.log("membersAfter "+JSON.stringify(membersAfter, null, 2))

  console.log('remove a member')
  await contracts.bioregion.join(bioname, thirduser, { authorization: `${thirduser}@active` })
  await contracts.bioregion.removemember(bioname, firstuser, thirduser, { authorization: `${firstuser}@active` })

  const membersAfterRemove = await getMembers()
  //console.log("membersAfterRemove "+JSON.stringify(membersAfterRemove, null, 2))

  const admin = seconduser

  let delayWorks = true
  try {
    await contracts.bioregion.join(bioname, seconduser, { authorization: `${seconduser}@active` })
    delayWorks = false
  } catch (err) {}

  console.log('configure bio.vote.del to 0')
  await contracts.settings.conffloat("bio.vote.del", 0, { authorization: `${settings}@active` })
  await sleep(1000)


  console.log('add role')
  await contracts.bioregion.join(bioname, seconduser, { authorization: `${seconduser}@active` })

  await contracts.bioregion.addrole(bioname, firstuser, admin, "admin", { authorization: `${firstuser}@active` })

  const roles = await getRoles()
  //console.log("roles "+JSON.stringify(roles, null, 2))

  console.log('remove role')
  await contracts.bioregion.removerole(bioname, firstuser, admin, { authorization: `${firstuser}@active` })
  const rolesAfter = await getRoles()
  //console.log("rolesAfter "+JSON.stringify(rolesAfter, null, 2))

  await contracts.bioregion.join(bioname, fourthuser, { authorization: `${fourthuser}@active` })
  await contracts.bioregion.addrole(bioname, firstuser, fourthuser, "admin", { authorization: `${firstuser}@active` })
  await contracts.bioregion.leaverole(bioname, fourthuser, { authorization: `${fourthuser}@active` })
  const rolesAfter2 = await getRoles()

  console.log('admin')
  await contracts.bioregion.addrole(bioname, firstuser, seconduser, "admin", { authorization: `${firstuser}@active` })
  await contracts.bioregion.join(bioname, thirduser, { authorization: `${thirduser}@active` })

  console.log('admin removes member')
  const membersBeforeAdminRemove = await getMembers()
  await contracts.bioregion.removemember(bioname, seconduser, thirduser, { authorization: `${seconduser}@active` })
  const membersAfterAdminRemove = await getMembers()
  //console.log("membersAfterAdminRemove "+JSON.stringify(membersAfterAdminRemove, null, 2))

  var cantremove = true
  try {
    await contracts.bioregion.removemember(bioname, seconduser, firstuser, { authorization: `${seconduser}@active` })
    cantremove = false
  } catch { }
  var cantremovefounder = true
  try {
    await contracts.bioregion.removemember(bioname, firstuser, firstuser, { authorization: `${seconduser}@active` })
    cantremovefounder = false
  } catch { }
  
  var cantaddrole = true
  try {
    await contracts.bioregion.addrole(bioname, seconduser, thirduser, "founder")
    cantaddrole = false
  } catch { }

  var cantsetfounder = true
  try {
    await contracts.bioregion.setfounder(bioname, seconduser, seconduser, { authorization: `${seconduser}@active` })
    cantsetfounder = false
  } catch { }

  await contracts.bioregion.setfounder(bioname, firstuser, seconduser, { authorization: `${firstuser}@active` })
  const bioregionsFounder = await getTableRows({
    code: bioregion,
    scope: bioregion,
    table: 'bioregions',
    json: true
  })

  assert({
    given: 'change founder',
    should: 'have different foudner',
    actual: bioregionsFounder.rows[0].founder,
    expected: seconduser
  })
  



assert({
  given: 'create bioregion',
  should: 'have bioregion',
  actual: bioregions.rows.length,
  expected: 1
})

assert({
  given: 'create bioregion',
  should: 'have member',
  actual: initialMembers.rows.length,
  expected: 1
})

assert({
  given: 'join member',
  should: 'have one more member',
  actual: members.rows.length,
  expected: 2
})

assert({
  given: 'leave',
  should: 'have one less member',
  actual: membersAfter.rows.length,
  expected: 1
})

assert({
  given: 'member removed',
  should: 'have one less member',
  actual: membersAfterRemove.rows.length,
  expected: 1
})

assert({
  given: 'add role',
  should: 'have role',
  actual: roles.rows.map(({account})=>account),
  expected: [
    firstuser,
    seconduser,
]
})
assert({
  given: 'remove role',
  should: 'have role',
  actual: rolesAfter.rows.map(({account})=>account),
  expected: [
    firstuser,
  ]
})

assert({
  given: 'remove admin',
  should: 'have one less member',
  actual: membersBeforeAdminRemove.rows.length - membersAfterAdminRemove.rows.length,
  expected: 1
})

assert({
  given: 'not allowed to remove member',
  should: 'be true',
  actual: cantremove,
  expected: true
})

assert({
  given: 'not allowed to add role',
  should: 'be true',
  actual: cantaddrole,
  expected: true
})

assert({
  given: 'not allowed to set founder',
  should: 'be true',
  actual: cantsetfounder,
  expected: true
})

assert({
  given: 'user left',
  should: 'have to wait for the delay to end',
  actual: delayWorks,
  expected: true
})


})

describe("Bioregions Test Delete", async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  const contracts = await initContracts({ accounts, bioregion, token, settings })

  const keypair = await createKeypair();

  await contracts.settings.reset({ authorization: `${settings}@active` })

  console.log('bioregion reset')
  await contracts.bioregion.reset({ authorization: `${bioregion}@active` })

  console.log('accounts reset')
  await contracts.accounts.reset({ authorization: `${accounts}@active` })

  console.log('configure - 1 Seeds feee')
  await contracts.settings.configure("bio.fee", 10000 * 1, { authorization: `${settings}@active` })

  console.log('join users')
  await contracts.accounts.adduser(firstuser, 'first user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(seconduser, 'second user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(thirduser, 'thirduser user', 'individual', { authorization: `${accounts}@active` })
  await contracts.accounts.adduser(fourthuser, 'second user', 'individual', { authorization: `${accounts}@active` })

  console.log('transfer fee for a bioregion')
  await contracts.token.transfer(firstuser, bioregion, "1.0000 SEEDS", "test", { authorization: `${firstuser}@active` })
  await contracts.token.transfer(seconduser, bioregion, "1.0000 SEEDS", "test", { authorization: `${seconduser}@active` })

  const bioname = "bdc1.bdc"
  const bioname2 = "bdc2.bdc"

  const getMembers = async () => {
    return await getTableRows({
      code: bioregion,
      scope: bioregion,
      table: 'members',
      json: true
    })
  }
  
  const getRoles = async (br) => {
    return await getTableRows({
      code: bioregion,
      scope: br,
      table: 'roles',
      json: true
    })
  }

  console.log('create a bioregion named '+bioname)

  await contracts.bioregion.create(
    firstuser, 
    bioname, 
    'test bio region',
    '{lat:0.0111,lon:1.3232}', 
    1.1, 
    1.23, 
    keypair.public, 
    { authorization: `${firstuser}@active` })

    lat = 39.0894
    lon = 1.4539

    console.log('create a second bioregion named '+bioname2)

    await contracts.bioregion.create(
      seconduser, 
      bioname2, 
      'test bio region 1',
      '{lat:0.0111,lon:1.3232}', 
      1.1, 
      1.23, 
      keypair.public, 
      { authorization: `${seconduser}@active` })
  
  
  console.log('join a bioregion')
  await contracts.bioregion.join(bioname2, fourthuser, { authorization: `${fourthuser}@active` })
  await contracts.bioregion.join(bioname, thirduser, { authorization: `${thirduser}@active` })

  console.log('add role')
  await contracts.bioregion.addrole(bioname, firstuser, thirduser, "admin", { authorization: `${firstuser}@active` })
  await contracts.bioregion.addrole(bioname2, seconduser, fourthuser, "admin", { authorization: `${seconduser}@active` })

  const membersBefore = await getMembers()
  const roles1before = await getRoles(bioname)
  const roles2before = await getRoles(bioname2)

  // console.log("membersBefore "+JSON.stringify(membersBefore, null, 2))
  // console.log("roles1before "+JSON.stringify(roles1before, null, 2))
  // console.log("roles2before "+JSON.stringify(roles2before, null, 2))

  await contracts.bioregion.removebr(bioname2, { authorization: `${bioregion}@active` })

  const roles1After = await getRoles(bioname)
  const roles2After = await getRoles(bioname2)
  const membersAfter = await getMembers()

  // console.log("membersAfter "+JSON.stringify(membersAfter, null, 2))
  // console.log("roles2After "+JSON.stringify(roles2After, null, 2))
  // console.log("roles1After "+JSON.stringify(roles1After, null, 2))

  assert({
    given: 'remove bioregion 2',
    should: 'bioregion 1 unaffected',
    actual: roles1before.rows.length == roles1After.rows.length,
    expected: true
  })

  assert({
    given: 'remove bioregion 2',
    should: 'bioregion 2 removed roles',
    actual: roles2before.rows.length - roles2After.rows.length,
    expected: 2
  })
    
assert({
  given: 'remove bioregion 2',
  should: 'have less members',
  actual: membersBefore.rows.length - membersAfter.rows.length,
  expected: 2
})


})
