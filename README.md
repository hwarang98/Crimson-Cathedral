# Arc: The Crimson Cathedral

Unreal Engine 5와 C++ 기반으로 개발한 **3인칭 소울라이크 액션 RPG** 프로젝트입니다.

Gameplay Ability System(GAS)을 중심으로 전투 시스템을 설계하고  
Motion Matching 기반 캐릭터 이동과 Gameplay Camera 시스템을 통해  
전투 상황에 최적화된 캐릭터 컨트롤과 시야를 구현했습니다.

본 프로젝트는 **7인 팀 프로젝트**로 진행되었으며  
저는 **프로젝트 리드 및 전투 시스템 / GAS 아키텍처 설계**를 담당했습니다.

---

# 🎮 Gameplay Video

[![Video Label](http://img.youtube.com/vi/EKn21if_jY8/0.jpg)](https://youtu.be/EKn21if_jY8)

---

# 📌 프로젝트 정보

| 항목 | 내용 |
|---|---|
| 프로젝트 | Arc: The Crimson Cathedral |
| 장르 | 3인칭 액션 RPG (Soulslike) |
| 개발 인원 | 7명 |
| 개발 기간 | 2025.11 ~ 2026.01 |
| 개발 엔진 | Unreal Engine 5 |
| 언어 | C++, Blueprint |

---

# 👨‍💻 담당 역할

**팀 리드 / 전투 시스템 & GAS 설계**

- 프로젝트 초기 구조 설계 및 공통 시스템 구축
- GAS(Gameplay Ability System) 기반 전투 프레임워크 설계
- 플레이어 캐릭터 핵심 전투 메커니즘 구현
- Gameplay Camera 시스템 설계 및 전투 카메라 구조 구현
- CI/CD 파이프라인 구축 및 코드 품질 관리 기준 수립

---

# ⚙️ 주요 구현 시스템

## GAS (Gameplay Ability System)

프로젝트 전투 시스템을 **GAS 기반 아키텍처로 설계**했습니다.

### GAS 아키텍처 설계

- AbilitySystemComponent
- AttributeSet
- GameplayEffect
- GameplayTag

프로젝트 전반의 **Ability / Effect / Attribute 구조를 표준화**했습니다.

### 캐릭터 속성 시스템

- Health
- Mana
- Stamina

각 속성은 **GameplayEffect 기반으로 관리**하며  
커스텀 **데미지 계산 로직**을 구현했습니다.

### 무기 장착 Ability

무기 장착을 독립적인 GameplayAbility로 설계했습니다.

- 무기 스폰
- 무기 등록
- 무기 해제
- Damage Spec 적용
- Input Mapping Context 자동 적용

### 그로기 시스템

적의 **Groggy 상태를 GameplayTag 기반으로 관리**하는 Ability 구현

### 사망 Ability

- 캐릭터 사망 처리 Ability 구현
- 멀티플레이 환경을 고려한 Replication 처리
- 장착 무기 제거 로직 연동

---

# ⚔️ 전투 시스템

### 컴포넌트 기반 전투 아키텍처

전투 로직을 컴포넌트 단위로 분리했습니다.
```
Character
├ PlayerCombatComponent
├ EnemyCombatComponent
└ Weapon System
```

역할 분리

- 무기 충돌 처리
- 히트 판정
- 상호작용 로직

이를 통해 **전투 시스템 확장성과 유지보수성을 확보했습니다.**

### 콤보 공격 시스템

낭인(Blade) 캐릭터의 기본 공격 콤보 구현

- 입력 버퍼 기반 콤보 처리
- 연속 공격 흐름 제어

### 패링 시스템

적 공격 타이밍에 반응하는 **패링 GameplayAbility 구현**

### 차징 스킬 (Blade Q)

차징 기반 특수 스킬 Ability 구현

- 충전 단계별 스킬 분기
- GameplayTag 기반 상태 관리

### 무기 충돌 처리

AnimNotify 기반 무기 충돌 토글

- 공격 애니메이션 특정 프레임에서만 충돌 활성화
- 정확한 히트 판정 보장

### 스테미나 시스템

공격 및 스프린트 액션 시 **스테미나 자동 차감**

---

# 🎯 타겟 락온 시스템

근접 전투에 특화된 **Lock-On 시스템 구현**

- 타겟 탐색
- 락온 유지
- GameplayTag 기반 상태 관리

스프린트 상태와 연동하여 **성능 최적화** 수행

---

# 🏃 Motion Matching 이동 시스템

Unreal Engine 5 **Motion Matching 플러그인**을 활용하여  
캐릭터 이동 애니메이션을 구현했습니다.

구현 기능

- Walk
- Run
- Sprint

특징

- 이동 방향 기반 애니메이션 선택
- 자연스러운 캐릭터 이동 구현
- 전투 상태와 이동 상태 간 자연스러운 전환

---

# 🎥 Gameplay Camera 시스템

UE5 **Gameplay Camera 시스템을 도입**하여  
전투 상황에 맞는 카메라 동작을 구현했습니다.

### Camera Director 구조

카메라 상태 관리를 위해 **Camera Director 구조를 설계했습니다.**

Character State
↓
Camera Director
↓
Camera Mode
(Exploration / Combat / LockOn)


### Lock-On 카메라

- 플레이어와 적을 동시에 프레임에 유지
- 적 중심 LookAt 카메라
- 거리 기반 FOV 조정

### Gameplay Tag 기반 카메라 전환

캐릭터 상태에 따라 카메라 모드를 자동 전환

예
State.LockOn
State.Combat
State.HitReact


### 로컬 카메라 처리

카메라 시스템을 **클라이언트 로컬에서만 처리**

- 네트워크 부하 감소
- 클라이언트 반응성 향상

---

# 👑 보스 시스템

**Cursed King 중간 보스 구현**

구현 내용

- 보스 Blueprint 설계
- 보스 전용 Ability 구현
- 보스 애니메이션 및 공격 패턴 구현
- 보스 상태 GameplayTag 추가

---

# 💰 재화 & 상점 시스템

### 재화 시스템

- GAS Attribute 기반 Currency 시스템
- 적 처치 시 보상 지급

### 상점 시스템

- 구매 / 판매 기능 구현
- DataTable 기반 아이템 관리
- 구매 결과 UI 피드백
- 자동 UI 숨김(Auto Hide)

---

# 🧠 기술적 기여

| 항목 | 내용 |
|---|---|
| GAS 아키텍처 | 프로젝트 최초 GAS 프레임워크 설계 |
| 전투 컴포넌트 분리 | PlayerCombatComponent / EnemyCombatComponent 구조 설계 |
| 공용 유틸리티 | CMFunctionLibrary 기반 공통 게임플레이 로직 정리 |
| 팀 ID 중앙화 | 분산된 팀 ID 로직 리팩토링 |
| CI/CD 구축 | GitHub Actions + clang-format 기반 코드 스타일 검사 |
| 리플리케이션 | 사망 Ability 멀티플레이 동기화 구현 |
| 무기 스폰 공용화 | 무기 스폰 로직 Ability 기반 공용화 |

---

# 🛠 트러블 슈팅

## 무기 장착 상태 처리 버그

### 문제

무기 장착 전 상태에서 무기가 노출되는 버그 발생

### 해결

무기 장착 전 기본 상태 로직을 명확히 정의하고  
숨김/보임 로직을 단순화하여 분기 조건 정리

---

## Blade Q 스킬 버그

### 문제

차징 스킬 구현 이후 조건 분기 오류로 스킬이 비정상 발동

### 해결

Ability 상태를 GameplayTag 기반으로 관리하여 분기 안정화

---

## 무기 충돌 판정 오류

### 문제

의도하지 않은 프레임에서 무기 충돌 발생

### 해결

AnimNotify 기반 충돌 토글 적용

---

## 타겟 락온 성능 문제

### 문제

타겟 탐색 로직이 매 프레임 실행되어 성능 저하 발생

### 해결

GameplayTag 기반 상태 연동으로 탐색 조건 최적화

---

# 🧰 사용 기술

| 분류 | 기술 |
|---|---|
| Engine | Unreal Engine 5 |
| Language | C++, Blueprint |
| Gameplay Framework | Gameplay Ability System (GAS) |
| Animation | Motion Matching, AnimInstance, AnimNotify |
| Camera | Gameplay Camera System |
| Input | Enhanced Input |
| Combat Architecture | Component 기반 전투 시스템 |
| UI | UMG Widget Blueprint |
| Network | Replication |
| CI/CD | GitHub Actions, clang-format |
| Version Control | Git, GitHub |



