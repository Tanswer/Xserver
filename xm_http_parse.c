/*
 * @filename:    http_parse.c
 * @author:      Tanswer
 * @date:        2017年12月12日 22:11:27
 * @description:
 */

#include "xm_http.h"
#include "dbg.h"
#include <sys/types.h>

typedef  unsigned char u_char;


/* 小端 */
#define xm_str3_cmp(m, c0, c1, c2, c3)  \
    *(u_int32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define xm_str30cmp(m, c0, c1, c2, c3)  \
    *(u_int32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define xm_str4cmp(m, c0, c1, c2, c3)  \
    *(u_int32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)


// 使用状态机遵循两条重要原则：减少内存拷贝和回溯
// 在需要内存拷贝的地方尽量只拷贝内存的起始和结束地址，但是这样就不能修改内存本身

/*
 * GET /sample.jsp HTTP/1.1
 *  
 *  Accept:image/gif.image/jpeg,**
 *  Accept-Language:zh-cn
 *  Connection:Keep-Alive
 *  Host:localhost
 *  User-Agent:Mozila/4.0(compatible:MSIE5.01:Windows NT5.0)
 *  Accept-Encoding:gzip,deflate.
 */ //解析上面的GET /sample.jsp HTTP/1.1
/*
 * 返回值有三类
 * XM_OK    成功解析到完整的HTTP请求行
 * XM_AGAIN 目前接收到的字符流不足以构成完整的请求行
 * XM_HTTP_PARSE_INVALID_METHOD XM_HTTP_PARSE_INVALID_REQUEST 接收到非法的请求行
 */
int xm_http_parse_request_line(xm_http_request_t *r)
{
    u_char ch, *m,*p;
    size_t pi;

    enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } state;

    state = r -> state;

    for(pi = r->pos; pi < r->last; pi++){
        p = (u_char *)&r->buf[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                r -> request_start = p;

                if(ch == CR || ch == LF){
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_'){
                    return XM_HTTP_PARSE_INVALID_METHOD;
                }

                state = sw_method;
                break;
            
            case sw_method:
                if(ch == ' '){
                    r->method_end = p-1;
                    m = r->request_start;

                    switch(p - m) {
                        case 3:
                            if(xm_str3_cmp(m,'G','E','T',' ')){
                                r -> method = XM_HTTP_GET;
                                break;
                            }
                            
                            if(xm_str3_cmp(m,'P','U','T',' ')){
                                r -> method = XM_HTTP_PUT;
                                break;
                            }

                            break;
                        
                        case 4:
                            if(xm_str30cmp(m,'P','O','S','T')){
                                r->method = XM_HTTP_POST;
                                break;
                            }

                            if(xm_str4cmp(m,'H','E','A','D')){
                                r->method = XM_HTTP_HEAD;
                                break;
                            }

                            break;
                    }

                    state = sw_spaces_before_uri;
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_'){
                    return XM_HTTP_PARSE_INVALID_METHOD;
                }

                break;

            case sw_spaces_before_uri:
                if(ch == '/'){
                    r->uri_start = p;
                    state = sw_after_slash_in_uri;
                    break;
                }

                switch(ch){
                    case ' ':
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_after_slash_in_uri:

                switch(ch){
                    case ' ':
                        r->uri_end = p;
                        state = sw_http;
                        break;
                    default:
                        break;
                }
                break;

            case sw_http:
                switch(ch) {
                    case ' ':
                        break;
                    case 'H':
                        state = sw_http_H;
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_H:
                switch(ch){
                    case 'T':
                        state = sw_http_HT;
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HT:
                switch(ch){
                    case 'T':
                        state = sw_http_HTT;
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTT:
                switch(ch){
                    case 'P':
                        state = sw_http_HTTP;
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTTP:
                switch(ch){
                    case '/':
                        state = sw_first_major_digit;
                        break;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            /* first digit of major HTTP version */
            case sw_first_major_digit:
                if(ch < '1' || ch > '9'){
                    return XM_HTTP_PARSE_INVALID_REQUEST;
                }

                r->http_major = ch - '0';
                state = sw_major_digit;
                break;

            /* major HTTP version or dot */
            case sw_major_digit:
                if(ch == '.'){
                    state = sw_first_minor_digit;
                    break;
                }

                if(ch < '0' || ch > '9'){
                    return XM_HTTP_PARSE_INVALID_REQUEST;
                }

                r->http_major = r->http_major * 10 + ch - '0';
                break;

            /* first digit of minor HTTP version */
            case sw_first_minor_digit:
                if(ch < '0' || ch > '9'){
                    return XM_HTTP_PARSE_INVALID_REQUEST;
                }

                r -> http_minor = ch - '0';
                state = sw_minor_digit;
                break;

            case sw_minor_digit:
                if(ch == CR){
                    state = sw_almost_done;
                    break;
                }

                if(ch == LF){
                    goto done;
                }

                if(ch == ' '){
                    state = sw_spaces_after_digit;
                    break;
                }

                if(ch < '0' || ch > '9'){
                    return XM_HTTP_PARSE_INVALID_REQUEST;
                }

                r->http_minor = r->http_minor * 10 + ch - '0';
                break;

            case sw_spaces_after_digit:
                switch(ch){
                    case ' ':
                        break;
                    case CR:
                        state = sw_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            /* end of request line */
            case sw_almost_done:
                r->request_end = p -1;
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return XM_HTTP_PARSE_INVALID_REQUEST;
                }
        } /* end of switch */
    }/* end of for */

    r -> pos = pi;
    r -> state = state;

    return XM_AGAIN;    /* 该行内容不够，继续recv 然后接着上次解析的位置继续解析直到请求行解析完毕 */

done:
    r->pos = pi+1;
    
    if(r->request_end == NULL){
        r->request_end = p;
    }

    r -> state = sw_start;

    return XM_OK;
}           


/*
 * GET /sample.jsp HTTP/1.1
 *  
 *  Accept:image/gif.image/jpeg,**
 *  Accept-Language:zh-cn
 *  Connection:Keep-Alive
 *  Host:localhost
 *  User-Agent:Mozila/4.0(compatible:MSIE5.01:Windows NT5.0)
 *  Accept-Encoding:gzip,deflate.
 */ //解析上面的GET /sample.jsp HTTP/1.1以外的配置,每个 header line 都是 key:value 形式的
/*
 * 返回值
 * XM_OK    解析完所有的 header 
 * XM_HTTP_PARSE_INVALID_HEADER 出错
 */

int xm_http_parse_header_line(xm_http_request_t *r)
{
    u_char ch, *p;
    size_t pi;

    enum {
        sw_start = 0,
        sw_name,
        sw_space_before_colon,
        sw_space_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    }state;

    state = r->state;
    check((state ==0), "state should be zero");
    
    /* 保存每一个 header line */
    xm_http_header_t *hd;

    for(pi = r->pos; pi<r->last; pi++){
        p = (u_char *)&r->buf[pi % MAX_BUF];
        ch = *p;

        switch(ch){
            case sw_start:
                if(ch == CR || ch == LF){
                    break;
                }

                r -> cur_header_key_start = p;
                state = sw_name;
                break;

            case sw_name:
                if(ch == ' '){
                    r -> cur_header_key_end = p;
                    state = sw_space_before_colon;
                    break;
                }
                if(ch == ':'){
                    r -> cur_header_key_end = p;
                    state = sw_space_after_colon;
                    break;
                }
                break;
                
            case sw_space_before_colon:
                if(ch == ' '){
                    break;
                }else if(ch == ':'){
                    state = sw_space_after_colon;
                    break;
                }else{
                    return XM_HTTP_PARSE_INVALID_HEADER; 
                }

            case sw_space_after_colon:
                if(ch == ' '){
                    break;
                }

                state = sw_value;
                r -> cur_header_value_start = p;
                break;

            case sw_value:
                if(ch == CR){
                    r -> cur_header_value_end = p;
                    state = sw_cr;
                    break;
                }
                if(ch == LF){
                    r -> cur_header_value_end = p;
                    state = sw_crlf;
                    break;
                }

            case sw_cr:
                if(ch == LF){
                    state = sw_crlf;

                    /* save the current header */
                    hd = (xm_http_header_t *)malloc(sizeof(xm_http_header_t));
                    hd -> key_start     = r -> cur_header_key_start;
                    hd -> key_end       = r -> cur_header_key_end; 
                    hd -> value_start   = r -> cur_header_value_start;
                    hd -> value_end     = r -> cur_header_value_end;

                    list_add(&(hd->list),&(r->list));
                    
                    break;
                }else{
                    return XM_HTTP_PARSE_INVALID_HEADER;
                }

            case sw_crlf:
                if(ch == CR){
                    state = sw_crlfcr;
                }else {
                    r -> cur_header_key_start = p;
                    state = sw_name;
                }
                break;

            case sw_crlfcr:
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return XM_HTTP_PARSE_INVALID_HEADER;
                }
                break;
        } // end switch
    } // end for

    r -> pos = pi;
    r -> state = state;

    return XM_AGAIN;
done:
    r -> pos = pi+1;
    r -> state = sw_start;
    return XM_OK;
}
