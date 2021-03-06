/*
Copyright (c) 2017-2018 Origin Quantum Computing. All Right Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "OriginQuantumMachine.h"
#include "Factory.h"
#include "TraversalAlgorithm/QCircuitParse.h"
#include "QuantumInstructionHandle/X86QuantumGates.h"
OriginQVM::OriginQVM()
{
	return;
}

void OriginQVM::init()
{
	_Qubit_Pool = 
		Factory::
		QubitPoolFactory::GetFactoryInstance().
		GetPoolWithoutTopology(_Config.maxQubit);
	_CMem = 
		Factory::
		CMemFactory::GetFactoryInstance().
		GetInstanceFromSize(_Config.maxCMem);
    QProg  temp = CreateEmptyQProg();
    _QProgram = temp.getPosition();
    _G_QNodeMap.addNodeRefer(_QProgram);
	_QResult =
		Factory::
		QResultFactory::GetFactoryInstance().
		GetEmptyQResult();
	_QMachineStatus =
		Factory::
		QMachineStatusFactory::
		GetQMachineStatus();
    _pGates = new X86QuantumGates();
}

Qubit * OriginQVM::Allocate_Qubit()
{
	if (_Qubit_Pool == nullptr)
	{
		// check if the pointer is nullptr
		// Before init
		// After finalize
		throw(invalid_pool());
	}
	else
	{
		return _Qubit_Pool->Allocate_Qubit();
	}
		
}

CBit * OriginQVM::Allocate_CBit()
{
	if (_CMem == nullptr)
	{
		// check if the pointer is nullptr
		// Before init
		// After finalize
		throw(invalid_cmem());
	}
	else
	{
		return _CMem->Allocate_CBit();
	}
}

void OriginQVM::Free_Qubit(Qubit *qubit)
{
    this->_Qubit_Pool->Free_Qubit(qubit);
}

void OriginQVM::Free_CBit(CBit *cbit)
{
    this->_CMem->Free_CBit(cbit);
}

void OriginQVM::load(QProg &loadProgram)
{
    QNodeAgency temp(&loadProgram, nullptr, nullptr);
    if (!temp.verify())
    {
		throw load_exception();
    }
    _G_QNodeMap.deleteNode(_QProgram);
    _QProgram = loadProgram.getPosition();
    _G_QNodeMap.addNodeRefer(_QProgram);
}

void OriginQVM::append(QProg & prog)
{
    QNodeAgency tempAgency(&prog, nullptr, nullptr);
    if (!tempAgency.verify())
    {
        throw load_exception();
    }
    auto aiter = _G_QNodeMap.getNode(_QProgram);
    if (nullptr == aiter)
        throw circuit_not_found_exception("cant found this QProgam", false);
    AbstractQuantumProgram * temp = dynamic_cast<AbstractQuantumProgram *>(aiter);
    temp->pushBackNode(&prog);
}

void OriginQVM::run()
{
    if (_QProgram < 0)
    {
        throw circuit_not_found_exception("QProgram cant found",false);
    }
    
    auto aiter =_G_QNodeMap.getNode(_QProgram);
    if (nullptr == aiter)
        throw circuit_not_found_exception("there is no this Qprog",false);

    QProg * pNode = (QProg *)aiter;
    _pParam = new QuantumGateParam();

	/* error may occur if user free working qubit before run() */
    _pParam->mQuantumBitNumber = _Qubit_Pool->getMaxQubit(); // -_Qubit_Pool->getIdleQubit();


    _pGates->initState(_pParam);
    QNodeAgency temp(pNode, _pParam, _pGates);
    if (temp.executeAction())
    {
		/* aiter has been used in line 120 */
        for (auto aiter : _pParam->mReturnValue)
        {
            _QResult->append(aiter);
        }
    }
    else
    {
        cout << "Warning:there is nothing in QProgram" << endl;
    }
    _pGates->endGate(nullptr,nullptr);
    delete _pParam;
    _pParam = nullptr;
    return ;

}

QMachineStatus * OriginQVM::getStatus() const
{
	return _QMachineStatus;
}

QResult * OriginQVM::getResult()
{
	return _QResult;
}

void OriginQVM::finalize()
{
    _G_QNodeMap.deleteNode(_QProgram);
	delete _Qubit_Pool;
	delete _CMem;
	delete _QResult;
	delete _QMachineStatus;
    delete _pGates;

}

size_t OriginQVM::getAllocateQubit()
{
	return _Qubit_Pool->getMaxQubit() - _Qubit_Pool->getIdleQubit();
}

size_t OriginQVM::getAllocateCMem()
{
	return _CMem->getMaxMem()- _CMem->getIdleMem();
}



