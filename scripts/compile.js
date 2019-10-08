const { exec } = require('child_process')
var fs = require('fs');
var dir = './tmp';

const command = ({ contract, source, dir }) => {
    const volume = dir
    //const cmd = `docker run --rm --name eosio.cdt_v1.6.1 --volume ${volume}:/project -w /project eostudio/eosio.cdt:v1.6.1 /bin/bash -c "echo 'starting';eosio-cpp -abigen -I ./include -contract ${contract} -o ./artifacts/${contract}.wasm ${source}"`
    const cmd = "eosio-cpp -abigen -I ./include -contract " + contract + " -o ./artifacts/"+contract+".wasm "+source;
    console.log("command: " + cmd);
    
    return cmd
}

const compile = ({ contract, source }) => {
    return new Promise((resolve, reject) => {

        // make sure source exists
        if (!fs.existsSync(source)) {
          throw new Error('Contract not found: '+contract+' No source file: '+source);
        }

        const dir = process.cwd() + "/"
        // check directory
        if (!dir.endsWith("seeds-contracts/")) {
          console.log("You have to run from seeds-contracts directory")
          return reject(null)
        }
        const artifacts = dir + "artifacts"
        
        // make sure artifacts exists
        if (!fs.existsSync(artifacts)){
          console.log("creating artifacts directory...")
          fs.mkdirSync(artifacts);
        }

        // clean build folder
        deleteIfExists(artifacts+"/"+contract+".wasm")
        deleteIfExists(artifacts+"/"+contract+".abi")

        // run compile        
        exec(command({ contract, source, dir }), (error, stdout, stderr) => {
          if (error) return reject(error)

  return new Promise((resolve, reject) => {
      exec(command({ contract, source }), (error, stdout, stderr) => {
        if (error) return reject(error)

        resolve()
      })
  })
}

const deleteIfExists = (file) => {
        if (fs.existsSync(file)){
          try {
            fs.unlinkSync(file)
            console.log("deleted existing ", file)
          } catch(err) {
            console.error("delete file error: "+err)
          }
        }
}

module.exports = compile