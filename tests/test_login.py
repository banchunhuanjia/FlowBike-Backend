import requests
import sys

# 尝试导入 bike_pb2
try:
    import bike_pb2
except ImportError:
    print("错误: 找不到 bike_pb2.py")
    print("请先执行命令: protoc -I=protos --python_out=. --experimental_allow_proto3_optional protos/bike.proto")
    sys.exit(1)


def run_test():
    # ================= 配置区 =================
    HOST = "127.0.0.1"
    PORT = 8080


    API_PATH = "/api/login"
    # =========================================

    url = f"http://{HOST}:{PORT}{API_PATH}"
    print(f"[-] 准备发送请求到: {url}")

    req = bike_pb2.login_request()
    req.mobile = "13800138000"
    req.icode = 6666

    # 序列化
    binary_data = req.SerializeToString()
    print(f"[-] 构造数据成功，二进制长度: {len(binary_data)} 字节")
    print(f"[-] 数据 Hex: {binary_data.hex()}")

    try:
        headers = {
            "Content-Type": "application/x-protobuf",
            "Connection": "keep-alive"
        }

        # 发送请求
        response = requests.post(url, data=binary_data, headers=headers, timeout=5)

        print(f"\n[+] 服务器响应状态码: {response.status_code}")

        if response.status_code == 200:
            resp_pb = bike_pb2.login_response()
            try:
                resp_pb.ParseFromString(response.content)
                print("[+] 解析响应成功!")
                print(f"    Code (状态码): {resp_pb.code}")
                print(f"    Data: {resp_pb.desc}")

                # --- 生成 wrk Lua 字符串 ---
                print("\n" + "=" * 50)
                print("【给 wrk 压测用的 Lua 字符串】(复制下面这一行):")
                lua_str = ""
                for byte in binary_data:
                    lua_str += f"\\x{byte:02x}"
                print(f'wrk.body = "{lua_str}"')
                print("=" * 50)

            except Exception as e:
                print(f"[!] 状态码200，但Protobuf反序列化失败: {e}")
                print(f"[!] 原始内容: {response.content}")
        elif response.status_code == 404:
            print(f"[x] 404 Not Found - 请检查 NetworkInterface::init_router 里有没有映射 {API_PATH}")
        else:
            print(f"[x] 请求失败，状态码: {response.status_code}")
            print(f"[x] 响应内容: {response.text}")

    except requests.exceptions.ConnectionError:
        print(f"[x] 连接失败！请检查 C++ 服务器是否启动，端口 {PORT} 是否正确。")
    except Exception as e:
        print(f"[x] 发生错误: {e}")


if __name__ == "__main__":
    run_test()