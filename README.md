# Zero-Copy SPSC Queue

<img width="423" height="100" alt="Screenshot 2026-08-27 172531" src="https://github.com/user-attachments/assets/c832c35c-5828-42f7-be3c-d0083d2fe797" />

<img width="417" height="100" alt="Screenshot 2026-08-27 172840" src="https://github.com/user-attachments/assets/4da2643a-337b-48b5-a193-57f5f51ea25f" />

The relevant queue here is the zero-copy cached queue featuring a two-stage API for producing and consuming messages. This two-stage process allows callers to produce and consume directly to and from the queue, without unnecessary copies. Large messages benefit greatly from this change, as you can see in the (rough!) benchmarks above.
