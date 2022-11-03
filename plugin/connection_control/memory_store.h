#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H
#include <map>
#include <iostream>
#include<algorithm>
#include<functional>
template <typename K, typename V> class Memory_store
{
private:
  std::map<K, V> m_dataMap;

public:
  void add(K name, V value) { m_dataMap.insert(std::pair<K, V>(name, value)); }
  void put(K name, V value) { m_dataMap[name]= value; }
  V find(K name)
  {
    auto it= m_dataMap.find(name);
    return it->second;
  }
  bool contains(K name) { return m_dataMap.find(name) != m_dataMap.end(); }

  bool erase(K name)
  {
    auto it= m_dataMap.find(name);
    if (it != m_dataMap.end())
    {
      m_dataMap.erase(it);
    }
    return true;
  }
  void foreach(std::function<void (std::pair<K,V>)> const &f)  {
    std::for_each(m_dataMap.cbegin(),m_dataMap.cend(),f);
  }
  
  void print()
  {
    // m_dataMap.insert(pair<K,V>(name,value));
    for (auto iter= m_dataMap.begin(); iter != m_dataMap.end(); ++iter)
    {
      std::cout << "Failed login" << iter->first << " " << iter->second
                << std::endl;
    }
  }
};
#endif
