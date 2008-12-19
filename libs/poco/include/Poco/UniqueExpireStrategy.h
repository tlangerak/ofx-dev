//
// UniqueExpireStrategy.h
//
// $Id: //poco/1.3/Foundation/include/Poco/UniqueExpireStrategy.h#1 $
//
// Library: Foundation
// Package: Cache
// Module:  UniqueExpireStrategy
//
// Definition of the UniqueExpireStrategy class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef  Foundation_UniqueExpireStrategy_INCLUDED
#define  Foundation_UniqueExpireStrategy_INCLUDED


#include "Poco/KeyValueArgs.h"
#include "Poco/ValidArgs.h"
#include "Poco/AbstractStrategy.h"
#include "Poco/Bugcheck.h"
#include "Poco/Timestamp.h"
#include "Poco/EventArgs.h"
#include <set>
#include <map>


namespace Poco {


template < 
	class TKey,
	class TValue
>
class UniqueExpireStrategy: public AbstractStrategy<TKey, TValue>
	/// An UniqueExpireStrategy implements time based expiration of cache entries. In contrast
	/// to ExpireStrategy which only allows to set a per cache expiration value, it allows to define 
	/// expiration per CacheEntry.
	/// Each TValue object must thus offer the following method:
	///    
	///    const Poco::Timestamp& getExpiration() const;
	///    
	/// which returns the absolute timepoint when the entry will be invalidated.
{
public:
	typedef std::multimap<Timestamp, TKey>     TimeIndex;
	typedef typename TimeIndex::iterator       IndexIterator;
	typedef typename TimeIndex::const_iterator ConstIndexIterator;
	typedef std::map<TKey, IndexIterator>      Keys;
	typedef typename Keys::iterator            Iterator;

public:
	UniqueExpireStrategy()
		/// Create an unique expire strategy.
	{
	}

	~UniqueExpireStrategy()
	{
	}

	void onAdd(const void*, const KeyValueArgs <TKey, TValue>& args)
	{
		// note: we have to insert even if the expire timepoint is in the past (for StrategyCollection classes to avoid inconsistency with LRU)
		// no problem: will be removed with next get
		const Timestamp& expire = args.value().getExpiration();
		IndexIterator it = _keyIndex.insert(std::make_pair(expire, args.key()));
		std::pair<Iterator, bool> stat = _keys.insert(std::make_pair(args.key(), it));
		if (!stat.second)
		{
			_keyIndex.erase(stat.first->second);
			stat.first->second = it;
		}
	}

	void onRemove(const void*, const TKey& key)
	{
		Iterator it = _keys.find(key);
		if (it != _keys.end())
		{
			_keyIndex.erase(it->second);
			_keys.erase(it);
		}
	}

	void onGet(const void*, const TKey& key)
	{
		// get triggers no changes in an expire
	}

	void onClear(const void*, const EventArgs& args)
	{
		_keys.clear();
		_keyIndex.clear();
	}

	void onIsValid(const void*, ValidArgs<TKey>& args)
	{
		Iterator it = _keys.find(args.key());
		if (it != _keys.end())
		{
			Timestamp now;
			if (it->second->first <= now)
			{
				args.invalidate();
			}
		}
	}

	void onReplace(const void*, std::set<TKey>& elemsToRemove)
	{
		// Note: replace only informs the cache which elements
		// it would like to remove!
		// it does not remove them on its own!
		IndexIterator it = _keyIndex.begin();
		Timestamp now;
		while (it != _keyIndex.end() && it->first < now)
		{
			elemsToRemove.insert(it->second);
			++it;
		}
	}

private:
	Keys      _keys;     /// For faster replacement of keys, the iterator points to the _keyIndex map
	TimeIndex _keyIndex; /// Maps time to key value
};


} // namespace Poco


#endif