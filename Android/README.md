### 😀Hello everyone, this is 🏢Agora Android team.

### 🔥This project is Next-Generation API Example of Agora SDK.

## All available demos ( for now )
- [Normal Audio Call][basicAudio]
- [Normal Video Call][basicVideo]
- [MediaPlayer][mediaPlayer]
- [screenShare][screenShare]
- [simpleExtension][simpleExtension]

## Getting Started

- NDK
> Since this project needs NDK support, you may encountered with some weird problem.
>
> If you already have a exist NDK version you can specified it in local.properties,
> like `ndk.dir=/Users/AgoraAndroidTeam/Library/Android/sdk/ndk/1.2.3456789`.
>
> Or else you need to grab one in Appearance & Behavior -> System Settings -> Android SDK -> SDK tools -> NDK .

- Agora SDK
> You may have problem with the SDK integration.
>
> If the SDK can fetched from maven, just add it.
>
> If you have a `zip` SDK file, extract the `.so` part to `app/src/main/jniLibs/`, jar file to `app/libs/`.
>
> Then sync project.

<br/>
<br/>

[basicAudio]: app/src/main/java/io/agora/ng_api/ui/fragment/BasicAudioFragment.java
[basicVideo]: app/src/main/java/io/agora/ng_api/ui/fragment/BasicVideoFragment.java
[mediaPlayer]: app/src/main/java/io/agora/ng_api/ui/fragment/MediaPlayerFragment.java
[screenShare]: app/src/main/java/io/agora/ng_api/ui/fragment/ScreenShareFragment.java
[simpleExtension]: app/src/main/java/io/agora/ng_api/ui/fragment/SimpleExtensionFragment.java