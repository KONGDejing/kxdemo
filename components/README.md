# components

## 组件依赖关系

```mermaid
graph TB
    dnesp32s3_bsp
    kxchat
    kxmqtt
    kxota
    kxui
    kxutils
    netcm

    kxutils --> kxmqtt
    kxutils --> kxchat
```
