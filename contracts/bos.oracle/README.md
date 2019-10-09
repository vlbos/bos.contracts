

### 文档

* [使用文档](https://github.com/vlbos/Documentation-1/blob/master/Oracle/bos_oracle_readme.md)
* [部署文档](https://github.com/vlbos/Documentation-1/blob/master/Oracle/bos_oracle_deployment.md)
* [一个简单的bos oracle 使用流程](https://github.com/vlbos/Documentation-1/blob/master/Oracle/bos_oracle_using_process.pdf)

### 模块


* [数据提供者--bos.provider.cpp](https://github.com/boscore/bos.contracts/tree/oracle.bos/contracts/bos.oracle/src/bos.provider.cpp)
* [数据使用者--bos.consumer.cpp](https://github.com/boscore/bos.contracts/tree/oracle.bos/contracts/bos.oracle/src/bos.consumer.cpp)
* [仲裁--bos.arbitration.cpp](https://github.com/boscore/bos.contracts/tree/oracle.bos/contracts/bos.oracle/src/bos.arbitration.cpp)
* [风控--bos.riskcontrol.cpp](https://github.com/boscore/bos.contracts/tree/oracle.bos/contracts/bos.oracle/src/bos.riskcontrol.cpp)
* [计费统计--bos.fee.cpp](https://github.com/boscore/bos.contracts/tree/oracle.bos/contracts/bos.oracle/src/bos.fee.cpp)


### 源目录结构

```
├── include  
│   └── bos.oracle 
│       ├── bos.constants.hpp 
│       ├── bos.functions.hpp  
│       ├── bos.oracle.hpp  
│       ├── bos.util.hpp  
│       ├── example  
│       │   └── consumer.contract.hpp  
│       ├── murmurhash.hpp  
│       └── tables  
│           ├── arbitration.hpp 
│           ├── consumer.hpp  
│           ├── oracle_api.hpp 
│           ├── provider.hpp 
│           └── riskcontrol.hpp 
└── src 
    ├── bos.arbitration.cpp 
    ├── bos.consumer.cpp 
    ├── bos.fee.cpp 
    ├── bos.oracle.cpp 
    ├── bos.provider.cpp 
    ├── bos.riskcontrol.cpp 
    ├── bos.util.cpp 
    └── example 
        └── consumer.contract.cpp 
```