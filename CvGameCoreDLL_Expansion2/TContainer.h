//------------------------------------------------------------------------------------------------
//
//  replacement for FFreeListTrashArray
//	store items with global IDs in a std::map
//	define iteration order in a vector 
//  
//------------------------------------------------------------------------------------------------

#ifndef		TCONTAINER_H
#define		TCONTAINER_H
#pragma		once

#include <map>
#include <list>

//helper function to generate unique IDs
int GetNextGlobalID();
//helper function to generate a pseudorandom number
int GetJonRand(int iRange);

//the ordered container
template <class T>
class TContainer
{
public:

	TContainer();
	virtual ~TContainer();
	
	//access the items directly by ID
	T* Get(int iID) const;
	bool Remove(int iID);

	//access the items indirectly by index
	T* GetAt(int iIndex) const;
	bool RemoveAt(int iIndex);
	int GetIndexForID(const int iID) const;

	//create a new item
	T* Add();
	//take over ownership of the pointer
	void Load(T* pExistingItem);

	//housekeeping
	int GetCount() const;
	int GetIndexAfterLast() const;
	void RemoveAll();

	//generic ordering
	void OrderShuffled();
	void OrderByID();
	template <class C>
	void OrderByContent(const C& comparator);

	//serialization
	void Read( FDataStream* pStream );
	void Write( FDataStream* pStream ) const;

private:
	//hide copy constructor and assignment operator
	TContainer(const TContainer& other) {}
	const TContainer& operator=(const TContainer& rhs) {}

protected:
	std::map<int,T*> m_items;
	std::vector<int> m_order;
};

template <class T>
TContainer<T>::TContainer()
{
}

template <class T>
TContainer<T>::~TContainer()
{
	//free all items
	for (std::map<int,T*>::iterator it=m_items.begin(); it!=m_items.end(); ++it)
		delete it->second;
}

template <class T>
T* TContainer<T>::Get(int iID) const
{
	std::map<int,T*>::const_iterator it=m_items.find(iID);

	if (it!=m_items.end())
		return it->second;

	return NULL;
}

template <class T>
T* TContainer<T>::GetAt(int iIndex) const
{
	std::map<int, T*>::const_iterator it = m_items.find(m_order[iIndex]);

	if (it != m_items.end())
		return it->second;

	return NULL;
}

template <class T>
bool TContainer<T>::Remove(int iID)
{
	std::vector<int>::iterator it=std::find(m_order.begin(),m_order.end(),iID);

	if (it!=m_order.end())
	{
		delete m_items[*it];
		m_items.erase( *it);
		m_order.erase( it );
		return true;
	}

	return false;
}

template <class T>
bool TContainer<T>::RemoveAt(int iIndex)
{
	if (iIndex>=0 && iIndex < (int)m_order.size())
	{
		delete m_items[ m_order[iIndex] ];
		m_items.erase( m_order[iIndex] );
		m_order.erase( m_order.begin() + iIndex );
		return true;
	}

	return false;
}

template <class T>
void TContainer<T>::RemoveAll()
{
	//free all items
	for (std::map<int,T*>::iterator it=m_items.begin(); it!=m_items.end(); ++it)
		delete it->second;

	m_items.clear();
	m_order.clear();
}

template <class T>
int TContainer<T>::GetIndexForID(const int iID) const
{
	std::vector<int>::const_iterator it=std::find(m_order.begin(),m_order.end(),iID);

	if (it!=m_order.end())
		return (int)(it-m_order.begin());

	return -1;
}

template <class T>
T* TContainer<T>::Add()
{
	T* pNewItem = new T;
	int iNewID = GetNextGlobalID();
	pNewItem->SetID( iNewID );
	m_order.push_back( iNewID );
	m_items[iNewID] = pNewItem;
	return pNewItem;
}

template <class T>
void TContainer<T>::Load(T* pExistingItem)
{
	if (pExistingItem)
	{
		m_order.push_back( pExistingItem->GetID() );
		m_items[pExistingItem->GetID()] = pExistingItem;
	}
}

template <class T>
int TContainer<T>::GetCount() const
{
	return (int)m_order.size();
}

template <class T>
int TContainer<T>::GetIndexAfterLast() const
{
	//last index is size-1, so next one is simply size
	return (int)m_order.size();
}

template <class T>
void TContainer<T>::OrderShuffled()
{
	//needs to be based on jonRand for reproducability
	std::random_shuffle( m_order.begin(), m_order.end(), GetJonRand );
}

template <class T>
void TContainer<T>::OrderByID()
{
	std::stable_sort( m_order.begin(), m_order.end() );
}

template <class T>
template <class C>
void TContainer<T>::OrderByContent(const C& comparator)
{
	std::stable_sort( m_order.begin(), m_order.end(), comparator );
}

//
// use when list contains non-streamable types
//
template < class T >
inline void TContainer< T >::Read( FDataStream* pStream )
{
	int nItems;
	pStream->Read(nItems);
	for ( int i=0; i<nItems; i++ )
	{
		T* pNewItem = new(T);
		pNewItem->Read(pStream);
		Load(pNewItem);
	}
}

template < class T >
inline void TContainer< T >::Write( FDataStream* pStream ) const
{
	pStream->Write(GetCount());
	for ( int i=0; i<GetCount(); i++ )
		GetAt(i)->Write(pStream);
}

//-------------------------------
// Serialization helper templates:
//-------------------------------

template<typename T>
FDataStream & operator<<(FDataStream & writeTo, const TContainer<T> & readFrom)
{
	writeTo << readFrom.GetCount();
	for ( int i=0; i<readFrom.GetCount(); i++ )
		writeTo << (*readFrom.GetAt(i));
	return writeTo;
}

template<typename T>
FDataStream & operator>>(FDataStream & readFrom, TContainer<T> & writeTo)
{
	int nItems;
	readFrom >> nItems;

	for (int i=0; i<nItems; i++)
	{
		T* pNewItem = new(T);
		readFrom >> (*pNewItem);
		writeTo.Load(pNewItem);
	}
	return readFrom;
}

#endif	//	TCONTAINER_H

