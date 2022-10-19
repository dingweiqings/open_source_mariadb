#include<map>
#include<iostream>
template<typename K,typename V>
class Memory_store{
private:
std::map<K,V> m_dataMap;    
public:
void add(K name,V value){
   m_dataMap.insert(std::pair<K,V>(name,value));
}
V find(K name){
    return m_dataMap.find(name)->second;
}
void print(){
            // m_dataMap.insert(pair<K,V>(name,value));
    for (auto iter = m_dataMap.begin(); iter != m_dataMap.end(); ++iter) {
        std::cout << "Failed login"<< iter->first << " " << iter->second << std::endl;
    }
}
};
