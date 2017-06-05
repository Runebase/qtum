#pragma once

#include <libethereum/State.h>
#include <libevm/ExtVMFace.h>
#include <crypto/sha256.h>
#include <crypto/ripemd160.h>
#include <uint256.h>
#include <primitives/transaction.h>
#include <qtum/qtumtransaction.h>

#include <libethereum/Executive.h>
#include <libethcore/SealEngine.h>

using OnOpFunc = std::function<void(uint64_t, uint64_t, dev::eth::Instruction, dev::bigint, dev::bigint, 
    dev::bigint, dev::eth::VM*, dev::eth::ExtVMFace const*)>;
using plusAndMinus = std::pair<dev::u256, dev::u256>;
using valtype = std::vector<unsigned char>;

struct TransferInfo{
    dev::Address from;
    dev::Address to;
    dev::u256 value;
};

struct Vin{
    dev::h256 hash;
    uint32_t nVout;
    dev::u256 value;
    uint8_t alive;
};

struct ResultExecute{
    dev::eth::ExecutionResult execRes;
    dev::eth::TransactionReceipt txRec;
    CTransaction tx;
};

namespace qtum{
    template <class DB>
    dev::AddressHash commit(std::unordered_map<dev::Address, Vin> const& _cache, dev::eth::SecureTrieDB<dev::Address, DB>& _state, std::unordered_map<dev::Address, dev::eth::Account> const& _cacheAcc)
    {
        dev::AddressHash ret;
        for (auto const& i: _cache){
            if(i.second.alive == 0){
                 _state.remove(i.first);
            } else {
                dev::RLPStream s(4);
                s << i.second.hash << i.second.nVout << i.second.value << i.second.alive;
                _state.insert(i.first, &s.out());
            }
            ret.insert(i.first);
        }
        return ret;
    }
}

class CondensingTX;

class QtumState : public dev::eth::State {
    
public:

    QtumState();

    QtumState(dev::u256 const& _accountStartNonce, dev::OverlayDB const& _db, const std::string& _path, dev::eth::BaseState _bs = dev::eth::BaseState::PreExisting);

    ResultExecute execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine, QtumTransaction const& _t, dev::eth::Permanence _p = dev::eth::Permanence::Committed, dev::eth::OnOpFunc const& _onOp = OnOpFunc());

    void setRootUTXO(dev::h256 const& _r) { cacheUTXO.clear(); stateUTXO.setRoot(_r); }

    dev::h256 rootHashUTXO() const { return stateUTXO.root(); }

    std::unordered_map<dev::Address, Vin> vins() const; // temp

    dev::OverlayDB const& dbUtxo() const { return dbUTXO; }

	dev::OverlayDB& dbUtxo() { return dbUTXO; }

    virtual ~QtumState(){}

    friend CondensingTX;

private:

    void transferBalance(dev::Address const& _from, dev::Address const& _to, dev::u256 const& _value);

    Vin const* vin(dev::Address const& _a) const;

    Vin* vin(dev::Address const& _addr);

    // void commit(CommitBehaviour _commitBehaviour);

    void kill(dev::Address _addr);

    void addBalance(dev::Address const& _id, dev::u256 const& _amount);

    dev::Address createQtumAddress(dev::h256 hashTx, uint32_t voutNumber);

    void deleteAccounts(std::vector<dev::Address>& addrs);

    void updateUTXO(const std::unordered_map<dev::Address, Vin>& vins);

    dev::Address newAddress;

    std::vector<TransferInfo> transfers;

    dev::OverlayDB dbUTXO;

	dev::eth::SecureTrieDB<dev::Address, dev::OverlayDB> stateUTXO;

	std::unordered_map<dev::Address, Vin> cacheUTXO;
};

///////////////////////////////////////////////////////////////////////////////////////////
class CondensingTX{

public:

    CondensingTX(QtumState* _state, const std::vector<TransferInfo>& _transfers, const QtumTransaction& _transaction, std::vector<dev::Address> _deleteAddresses = std::vector<dev::Address>()) : transfers(_transfers), transaction(_transaction), state(_state), deleteAddresses(_deleteAddresses){}

    CTransaction createCondensingTX();

    std::unordered_map<dev::Address, Vin> createVin(const CTransaction& tx);

private:

    void selectionVin();

    void calculatePlusAndMinus();

    bool createNewBalances();

    std::vector<CTxIn> createVins();

    std::vector<CTxOut> createVout();

    bool checkDeleteAddress(dev::Address addr);

    std::map<dev::Address, plusAndMinus> plusMinusInfo;

    std::map<dev::Address, dev::u256> balances;

    std::map<dev::Address, uint32_t> nVouts;

    std::map<dev::Address, Vin> vins;

    const std::vector<TransferInfo>& transfers;

    const std::vector<dev::Address> deleteAddresses;

    const QtumTransaction& transaction;

    QtumState* state;

};
///////////////////////////////////////////////////////////////////////////////////////////