package com.telematica.client;

import javax.swing.*;
import java.io.*;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.Map;

public class TelemetryClient {
    public interface Listener {
        void onConnected();
        void onLoginResult(boolean ok, String line);
        void onStatus(Map<String, String> kv);
        void onServerMessage(String line);
        void onError(String msg, Exception ex);
        void onClosed();
    }

    private final String host; private final int port; private final String role; private final String user; private final String pass;
    private final Listener listener;

    private volatile Socket socket; private volatile BufferedReader in; private volatile PrintWriter out;

    public TelemetryClient(String host, int port, String role, String user, String pass, Listener listener) {
        this.host = host; this.port = port; this.role = role; this.user = user; this.pass = pass; this.listener = listener;
    }

    public String getRole() {
        return role;
    }
    public boolean isObserver() {
        return role != null && role.equalsIgnoreCase("OBSERVER");
    }
    public boolean isAdmin() {
        return role != null && role.equalsIgnoreCase("ADMIN");
    }

    public boolean isConnected() { return socket != null && socket.isConnected() && !socket.isClosed(); }

    public void connectAsync() {
        new Thread(() -> {
            try {
                socket = new Socket(host, port);
                in = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
                out = new PrintWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8), true);

                SwingUtilities.invokeLater(listener::onConnected);

                // LOGIN
                String loginLine = String.format("LOGIN %s %s %s", role, user, pass);
                out.println(loginLine);

                // Primera línea: puede ser OK/ERR o telemetría (STATUS/DATA)
                String first = in.readLine();
                boolean ok = first != null && first.toUpperCase().startsWith("OK");
                boolean isTelemetry = first != null && (first.startsWith("STATUS ") || first.startsWith("DATA "));
                if (!isTelemetry) {
                    final String f = first != null ? first : "(sin respuesta)";
                    SwingUtilities.invokeLater(() -> listener.onLoginResult(ok, f));
                } else {
                    Map<String, String> kv = StatusParser.parse(first);
                    SwingUtilities.invokeLater(() -> listener.onStatus(kv));
                }

                // Loop receptor
                String line;
                while ((line = in.readLine()) != null) {
                    final String l = line;
                    if (l.startsWith("STATUS ") || l.startsWith("DATA ")) {
                        Map<String, String> kv = StatusParser.parse(l);
                        SwingUtilities.invokeLater(() -> listener.onStatus(kv));
                    } else {
                        SwingUtilities.invokeLater(() -> listener.onServerMessage(l));
                    }
                }
            } catch (Exception ex) {
                SwingUtilities.invokeLater(() -> listener.onError("Fallo de conexión/IO", ex));
            } finally {
                closeQuietly();
                SwingUtilities.invokeLater(listener::onClosed);
            }
        }, "net-recv").start();
    }

    public void send(String text) {
        if (out != null) out.print(text);
        if (out != null) out.flush();
    }

    private void closeQuietly() {
        try { if (in != null) in.close(); } catch (IOException ignored) {}
        try { if (out != null) out.close(); } catch (Exception ignored) {}
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
    }
}
