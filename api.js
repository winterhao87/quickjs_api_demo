console.log("Exchange Int:");
set_int(100);
console.log(get_int());

console.log("\nExchange String:");

set_str("name = ywh");
console.log(get_str());

console.log("\nExchange Array:");

var arr = [1, 2, "name", 3];
set_array(arr);
console.log(get_array());

console.log("\nExchange Object:");

set_object({});
var obj = {
  age: 10,
  name: "Winsen",
  play: function() {
    var a = "basketball";
    return a;
  },
  child: {
    sex: "boy",
    age: 0.1,
    getname: function() {
      return "handsome " + this.sex;
    },
  },
};
set_object(obj);

var obj = get_object();
console.log(obj);
console.log("age=", obj.age);
console.log("name=", obj.name);

console.log("\nExchange Opaque:");

var req = get_req("GET", "http://www.qq.com");
console.log("method=", req.get("method"));
console.log("url=", req.get("url"));

try {
  console.log("header=", req.get("host"));
} catch(e) {
  console.log(e.name + ": " + e.message);
  console.log(e.stack);
}

req.close();
req = null;

var t_req = new TestReq("POST", "http://www.baidu.com");
console.log("method=", t_req.get("method"));
console.log("url=", t_req.get("url"));
t_req.close();
t_req = null;

console.log("\nExchange JS Functon:");

function Hello(str) {
  console.log("hello " + str);
  return "[JS] Hello";
}

console.log(eval_js_func(Hello, "winsenye"));


console.log("\nClass Extend:");

class HttpReq extends TestReq{
    constructor(method, url, headers) {
        super(method, url);
        this.headers = headers;
    }

    get_method() {
      return this.get("method");
    }

    get_url() {
      return this.get("url");
    }

    get_headers() {
        return this.headers;
    }
};

var http_req = new HttpReq("HEAD", "http://www.jd.com", ["HOST: www.jd.com", "Content-Type: \"text/plain\""]);
console.log(http_req.__proto__);

console.log("method=", http_req.get_method());
console.log("url=", http_req.get_url());
console.log("headers=", http_req.get_headers());
// http_req.close();
http_req = null;


console.log("api done");
