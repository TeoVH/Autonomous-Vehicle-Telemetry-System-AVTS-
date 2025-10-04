#!/usr/bin/env python3
"""
Herramienta para generar hashes de contraseñas con salt para AVTS
Uso: python3 tools/hash_password.py --user juan --role OBSERVER --password 'secreto'
"""

import hashlib
import secrets
import json
import argparse
import sys

def generate_salt():
    """Genera un salt aleatorio de 32 bytes (64 caracteres hex)"""
    return secrets.token_hex(32)

def hash_password(password, salt):
    """Calcula SHA-256(salt + password)"""
    combined = salt + password
    return hashlib.sha256(combined.encode('utf-8')).hexdigest()

def create_user_entry(role, username, password):
    """Crea una entrada de usuario con hash y salt"""
    salt = generate_salt()
    password_hash = hash_password(password, salt)
    
    return {
        "role": role.upper(),
        "username": username,
        "salt": salt,
        "hash": password_hash
    }

def generate_example_config():
    """Genera config.json.example con usuarios predefinidos"""
    users = [
        create_user_entry("ADMIN", "root", "password"),
        create_user_entry("OBSERVER", "juan", "12345")
    ]
    
    config = {"users": users}
    return config

def main():
    parser = argparse.ArgumentParser(description='Generar hash de contraseña para AVTS')
    parser.add_argument('--user', required=False, help='Nombre de usuario')
    parser.add_argument('--role', required=False, help='Rol (ADMIN/OBSERVER)')
    parser.add_argument('--password', required=False, help='Contraseña')
    parser.add_argument('--json', action='store_true', help='Mostrar solo JSON')
    parser.add_argument('--example', action='store_true', help='Generar config.json.example')
    
    args = parser.parse_args()
    
    if args.example:
        config = generate_example_config()
        print(json.dumps(config, indent=2))
        return
    
    if not args.user or not args.role or not args.password:
        print("Error: Se requieren --user, --role y --password", file=sys.stderr)
        sys.exit(1)
    
    if args.role.upper() not in ['ADMIN', 'OBSERVER']:
        print("Error: role debe ser ADMIN u OBSERVER", file=sys.stderr)
        sys.exit(1)
    
    user_entry = create_user_entry(args.role, args.user, args.password)
    
    if args.json:
        print(json.dumps(user_entry, indent=2))
    else:
        print(f"Usuario: {user_entry['username']}")
        print(f"Rol: {user_entry['role']}")
        print(f"Salt: {user_entry['salt']}")
        print(f"Hash: {user_entry['hash']}")
        print("\nJSON para config.json:")
        print(json.dumps(user_entry, indent=2))

if __name__ == '__main__':
    main()