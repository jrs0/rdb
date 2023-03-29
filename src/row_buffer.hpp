#ifndef ROW_BUFFER_HPP
#define ROW_BUFFER_HPP

template<class T>
concept RowBuffer = requires(T t, const std::string & s) {
    t.size();
    //t.at(s);
    t.fetch_next_row();   
};


#endif
