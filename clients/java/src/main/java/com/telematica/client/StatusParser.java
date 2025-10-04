package com.telematica.client;

import java.util.*;

public class StatusParser {
    /**
     * Soporta:
     * 1) "STATUS speed=... battery=... dir=... temp=..."
     * 2) "DATA <speed> <battery> <dir> <temp>"
     */
    public static Map<String, String> parse(String line) {
        Map<String, String> map = new LinkedHashMap<>();
        if (line == null) return map;
        line = line.trim();

        if (line.startsWith("STATUS ")) {
            int idx = line.indexOf(' ');
            if (idx < 0 || idx >= line.length()-1) return map;
            String[] parts = line.substring(idx + 1).trim().split("\\s+");
            for (String p : parts) {
                int eq = p.indexOf('=');
                if (eq > 0 && eq < p.length()-1) {
                    String k = p.substring(0, eq).trim().toLowerCase();
                    String v = p.substring(eq+1).trim();
                    map.put(k, v);
                }
            }
            return map;
        }

        if (line.startsWith("DATA ")) {
            String[] parts = line.split("\\s+");
            // Esperado: DATA speed battery dir temp  (5 tokens)
            if (parts.length >= 5) {
                map.put("speed", parts[1]);
                map.put("battery", parts[2]);
                map.put("dir", parts[3]);
                map.put("temp", parts[4]);
            }
            return map;
        }

        return map;
    }
}
