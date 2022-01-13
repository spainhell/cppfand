#pragma once

template<class T>
class Chained
{
public:
	T* pChain = nullptr;
};

template<class T>
void ChainLast(Chained<T>* Frst, Chained<T>* New)
{
	//if (Frst == nullptr) throw std::exception("ChainLast: Parent is NULL!");

	if (New == nullptr) return;

	Chained<T>* last = Frst;

	while (last->pChain != nullptr) {
		last = last->pChain;
	}

	last->pChain = (T*)New;

	New->pChain = nullptr; // TODO: pridano kvuli zacykleni v RdAutoSortSK_M
}

template<class T>
Chained<T>* LastInChain(Chained<T>* Frst)
{
	Chained<T>* last = Frst->pChain;
	if (last == nullptr) {
		return Frst;
	}
	while (last != nullptr) {
		if (last->pChain == nullptr) return last;
		last = last->pChain;
	}
	return last;
}
