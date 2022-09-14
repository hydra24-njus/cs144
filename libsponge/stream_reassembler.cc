#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`


using namespace std;

#define first_unassembled  (_output.bytes_written())
#define first_unacceptalbe (_output.bytes_read()+_capacity)

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _buf() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof){
        _eof_index=index+data.length();
    }
    if(index<=first_unassembled){
        if(index+data.length()<first_unassembled)return;
        else{
            _output.write(data.substr(first_unassembled-index));
            while(!_buf.empty()&&_buf.begin()->first+_buf.begin()->second.length()<=first_unassembled)_buf.pop_front();
            if(!_buf.empty()&&_buf.begin()->first<=first_unassembled){
                _output.write(_buf.begin()->second.substr(first_unassembled-_buf.begin()->first));
                _buf.pop_front();
            }
            if(empty()&&first_unassembled==_eof_index){
                _output.end_input();
            }
        }
    }
    else if(index>=first_unacceptalbe)return;
    else{
        if(data.length()==0)return;
        //before
        string ndata=data.substr(0,first_unacceptalbe-index);
        if(_buf.empty()){_buf.push_back({index,ndata});return;}
        auto it=find_before(index);
        if(it==_buf.end())it=_buf.insert(_buf.begin(),{index,ndata});
        else if(it->first+it->second.length()>=index){
            if(it->first+it->second.length()>=index+ndata.length())return;
            it->second+=ndata.substr(it->first+it->second.length()-index);
        }
        else{
            it=_buf.insert(++it,{index,ndata});
        }
        //after
        auto nxt=++it;--it;
        while(nxt!=_buf.end()&&it->first+it->second.length()>=nxt->first){
            if(it->first+it->second.length()<nxt->first+nxt->second.length()){
                string remain=nxt->second.substr(it->first+it->second.length()-nxt->first);
                it->second+=remain;
            }
            _buf.erase(nxt);
            nxt=++it;--it;
        }
    }
    return;
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t ans=0;
    for(auto p=_buf.begin();p!=_buf.end();++p){
        ans+=p->second.length();
    }
    return ans;
}

bool StreamReassembler::empty() const { return _buf.empty(); }

list<pair<size_t,string>>::iterator StreamReassembler::find_before(size_t index){
    for(auto it=_buf.begin();it!=_buf.end();++it){
        if(it->first>index){
            if(it==_buf.begin())return _buf.end();
            return --it;
        }
    }
    return --_buf.end();
}