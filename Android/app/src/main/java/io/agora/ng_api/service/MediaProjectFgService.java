package io.agora.ng_api.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import java.util.Random;

import io.agora.ng_api.R;

public class MediaProjectFgService extends Service {
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopForeground(true);
    }

    private void createNotificationChannel() {

        String channelId = getString(R.string.notify_channel_id);

//        Starting from API 26
//        Notifications do not have NotificationChannel are not allowed to send
//        And will get a toast：No Channel found for xxx
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            int importance = NotificationManager.IMPORTANCE_HIGH;
            NotificationChannel channel = new NotificationChannel(channelId, MediaProjectFgService.class.getSimpleName(), importance);
            channel.setDescription(getString(R.string.notify_service_desc));
            channel.enableLights(true);
            channel.setLightColor(Color.RED);
            channel.enableVibration(true);
            NotificationManager notificationManager = (NotificationManager)
                    getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.createNotificationChannel(channel);
        }

        // Create a notification and set the notification channel.
        Notification notification = new NotificationCompat.Builder(this, channelId)
                .setContentText(getString(R.string.notify_recording_screen))
                .setBadgeIconType(NotificationCompat.BADGE_ICON_SMALL)
                .setSmallIcon(R.drawable.ic_icon_notification)
                .setColor(getResources().getColor(R.color.agora_blue))
                .setWhen(System.currentTimeMillis())
                .build();
        startForeground(new Random().nextInt(1024) + 1024, notification);
    }
}